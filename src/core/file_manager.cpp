#include <core/file_manager.h>
#include <format>
#include <google/protobuf/message.h>
#include <iostream>

using namespace std;
using namespace log_aggregator;
namespace NativeEvents = native::Events;

// ToDo: Add topic names and pass it to messenger so that it only subscribes to
// those topics.
FileManager::FileManager(shared_ptr<zmq::context_t> ctx,
                         ZmqSocketConfig recv_socket_config)
    : messenger_{ctx, recv_socket_config, recv_socket_callback()} {
  ctx_ = ctx;
  // Thread for processing events sent by File reader threads.
  event_processor_thread_ = jthread(&FileManager::process_events, this);
}

void FileManager::setup_message_sender(ZmqSocketConfig send_socket_config) {
  messenger_.setup_sender(send_socket_config);
}

MessageCallback FileManager::recv_socket_callback() {
  // Setting up event processor for events sent over ZMQ channel.
  return [this](const std::string &bytes) {
    schema::FileService envelope_event;
    if (envelope_event.ParseFromString(bytes)) {
      switch (envelope_event.payload_case()) {
      case schema::FileService::kFileEvents:
        break;
      case schema::FileService::kFileCommands:
        this->handle_file_commands(std::move(envelope_event.file_commands()));
        break;
      case schema::FileService::PAYLOAD_NOT_SET:
        cerr << "Unknown type of data received on ZMQ" << endl;
        break;
      }
    } else {
      cerr << "Could not parse: " << bytes << endl;
    }
  };
}

void FileManager::handle_file_commands(
    log_aggregator::schema::FileCommands file_command) {
  switch (file_command.command_type_case()) {
  case schema::FileCommands::kAddFile: {
    FileId file_id = file_command.add_file().id();
    auto result = add_file(file_id, file_command.add_file().path());
    if (!result) {
      string error = format("{}, {}", result.error().code.message(),
                            result.error().message);
      report_file_error(file_id, std::move(error));
    }
    break;
  }
  case schema::FileCommands::kCloseFile: {
    remove_file(file_command.close_file().id());
    break;
  }
  case schema::FileCommands::COMMAND_TYPE_NOT_SET:
    cerr << "Unknown type of Command received on ZMQ" << endl;
    break;
  }
}

Result<void, Error> FileManager::add_file(FileId file_id,
                                          const std::filesystem::path &path) {
  filesystem::path file_path{path};
  auto file_reader = FileReader::open_file(file_id, file_path);
  if (!file_reader)
    return unexpected(Error{file_reader.error(), ""});

  // Using mutex here because remove_file() also have access to the
  // file_readers_ and threads_ maps.
  {
    std::lock_guard lock(rw_mutex_);
    if (!file_readers_
             .try_emplace(file_id, make_shared<FileReader>(
                                       std::move(file_reader.value())))
             .second)
      return unexpected(Error::from_errno(EEXIST, "File already registered."));

    // Launch the file reader in a separate thread.
    jthread thread(&FileManager::process_file, this, file_id);
    if (!file_reader_threads_.try_emplace(file_id, std::move(thread)).second) {
      // No need to explicilty call thread.request_stop() since "thread" still
      // owns the jthread because move(thread) only happens if try_emplace
      // returns true. Hence on exit of this function, thread would call
      // request_stop() and join() itself.
      return unexpected(Error::from_errno(
          EEXIST, "A thread is already processing the file."));
    }
  }
  return {};
}

void FileManager::process_file(stop_token token, FileId id) {
  // This shareed_ptr ensures that reader stays alive for as long as run() needs
  // it. If we were to store the object in file_readers_ inside of a shared_ptr,
  // then removing the file reader from the map in remove_file() would have made
  // the run() here a dangling reference.
  shared_ptr<FileReader> reader;
  {
    // Using shared_lock here because we are only reading from the map.
    shared_lock lock(rw_mutex_);
    // Check if File Id exists because it is possible that the File Id was
    // deleted after adding the file but before launching the this thread on
    // file.
    auto it = file_readers_.find(id);
    if (it == file_readers_.end())
      return;
    reader = it->second;
  }
  reader->run(token, event_queue_);
  // Lock should be released before calling a function which depends on
  // stop_token to be interrupted because otherwise this lock would not get
  // acquired by a function which would set the stop_token and hence leading to
  // a deadlock.
  // reader->run(token, event_queue_);
  cout << "Thread closed" << endl;
  // The shared ptr to FileReader would be destroyed here.
}

void FileManager::remove_file(FileId file_id) {
  // Removing the thread from the map would triggers its destructor which would
  // request_stop(), setting the stop_token, and thread.join()
  jthread thread_to_stop;
  {
    // Using exclusive lock here since we are modifying the file_readers map.
    lock_guard lock(rw_mutex_);
    auto it = file_reader_threads_.find(file_id);
    if (it != file_reader_threads_.end()) {
      thread_to_stop = std::move(it->second);
      thread_to_stop.request_stop();
      // Moving it to another vector so that this function can return quickly
      // instead of waiting for the thread to join. Waiting for the thread to
      // join would block the UI when the user removes a file pane.
      orphaned_threads.push_back(std::move(thread_to_stop));
      file_reader_threads_.erase(it);
    }
    // Deleting the reference to FileReader held by the map.
    file_readers_.erase(file_id);
  }
}

void FileManager::process_events(stop_token token) {
  while (!token.stop_requested()) {
    visit([this](const auto &event) { handle(event); },
          event_queue_.pop(token));
  }
}

void FileManager::handle(const NativeEvents::InotifyError &event) {
  cerr << format("File ID: {}, error code: {}", event.id,
                 event.error_code.message())
       << endl;
  cerr << "Closing the File ..." << endl;
  remove_file(event.id);
  report_file_error(event.id, event.error_code.message());
}

void FileManager::handle(const NativeEvents::FileError &event) {
  cout << "File Error event" << endl;
  cout << format("Error: {}: {}", event.error.code.message(),
                 event.error.message);
  // ToDo: Refactor it in the future: show the error to the user and let them
  // decide if they want to close the file.
  cout << "Removing the file ..." << endl;
  remove_file(event.id);
  report_file_error(event.id, event.error.code.message());
}

void FileManager::handle(const NativeEvents::FileClosed &event) {
  cout << "File Closed event" << endl;
  cout << "Removing the file ..." << endl;
  remove_file(event.id);
  report_file_closed(event.id);
}

void FileManager::handle(const NativeEvents::DataAvailable &event) {
  // cout << "Data Available event: " << event.data << endl;
  report_data_available(event.id, std::move(event.data));
}

void FileManager::report_data_available(FileId file_id, const string data) {
  schema::FileEvents event;
  auto *data_event = event.mutable_data_available();
  data_event->set_id(file_id);
  data_event->set_data(std::move(data));

  messenger_.send(std::move(event));
}

void FileManager::report_file_error(FileId file_id, const string error) {
  schema::FileEvents event;
  auto *error_event = event.mutable_file_error();
  error_event->set_id(file_id);
  error_event->set_error(std::move(error));

  messenger_.send(std::move(event));
}

void FileManager::report_file_closed(FileId file_id) {
  schema::FileEvents event;
  auto *error_event = event.mutable_file_closed();
  error_event->set_id(file_id);

  messenger_.send(std::move(event));
}
