#pragma once
#include <core/events.h>
#include <core/file_reader.h>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <utils/config.h>
#include <utils/messenger.h>
#include <utils/thread_safe_queue.h>

class FileManager {
public:
  FileManager(std::shared_ptr<zmq::context_t>, ZmqSocketConfig);
  void start_event_processing();
  Result<FileId, log_aggregator ::Error>
  add_file(const std::filesystem::path &);
  void remove_file(FileId);
  void process_file(std::stop_token, FileId id);
  void process_events(std::stop_token);
  void handle(const native::Events::InotifyError &);
  void handle(const native::Events::FileError &);
  void handle(const native::Events::FileClosed &);
  void handle(const native::Events::DataAvailable &);

private:
  FileId next_file_id_{0};
  // Shared mutex for read/write operations on maps.
  std::shared_mutex rw_mutex_;
  Messenger messenger_;
  ThreadSafeQueue<FileProcessingEvent> event_queue_;
  std::jthread event_processor_thread_;
  // Storing shared_ptr instead of the object itself because it is required
  // independently at two places: in process_file() and remove_file().
  std::unordered_map<FileId, std::shared_ptr<FileReader>> file_readers_;
  std::unordered_map<FileId, std::jthread> file_reader_threads_;
  // detached but tracked threads.
  std::vector<std::jthread> orphaned_threads;
};
