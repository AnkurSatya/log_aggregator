#include <file_manager.h>
#include <file_reader.h>

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
    if (!file_readers_.try_emplace(file_id, std::move(file_reader.value()))
             .second)
      return unexpected(Error::from_errno(EEXIST, "File already registered."));
  }

  std::jthread thread(&FileManager::process_file, this, file_id);
  if (!file_reader_threads_.try_emplace(file_id, std::move(thread)).second) {
    thread.request_stop();
    return unexpected(
        Error::from_errno(EEXIST, "A thread is already processing the file."));
  }
  // Launch the file reader
  next_file_id_++;
  return next_file_id_;
}

void FileManager::process_file(std::stop_token token, FileId id) {
  std::shared_lock lock(rw_mutex_);
  // Check if File Id exists because it is possible that the File Id was deleted
  // after adding the file but before launching the this thread on file.
  auto it = file_readers_.find(id);
  if (it == file_readers_.end())
    return;
  it->second.run(token, event_queue_);
}
