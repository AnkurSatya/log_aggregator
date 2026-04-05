#include <file_manager.h>
#include <file_reader.h>
#include <iostream>

using namespace std;
using namespace log_aggregator;
Result<FileId, Error> FileManager::add_file(const std::filesystem::path path) {
  filesystem::path file_path{path};
  FileId file_id = next_file_id_;
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
      thread.request_stop();
      return unexpected(Error::from_errno(
          EEXIST, "A thread is already processing the file."));
    }
  }
  next_file_id_++;
  return file_id;
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
    reader->run(token, event_queue_);
  }
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
    std::lock_guard lock(rw_mutex_);
    auto it = file_reader_threads_.find(file_id);
    if (it != file_reader_threads_.end()) {
      thread_to_stop = std::move(it->second);
      file_reader_threads_.erase(it);
    }
    // Deleting the reference to FileReader held by the map.
    file_readers_.erase(file_id);
  }
  cout << "Joining thread" << endl;
}
