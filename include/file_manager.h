#pragma once
#include <file_reader.h>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <utils/events.h>
#include <utils/thread_safe_queue.h>
#include <vector>

class FileManager {
public:
  Result<FileId, log_aggregator ::Error> add_file(const std::filesystem::path);
  void remove_file(FileId);
  void process_file(std::stop_token, FileId id);
  void process_events(std::stop_token);
  void handle(const Events::FileError &);
  void handle(const Events::FileClosed &);
  void handle(const Events::DataAvailable &);

private:
  FileId next_file_id_{0};
  ThreadSafeQueue<FileProcessingEvent> event_queue_;
  std::unordered_map<FileId, FileReader> file_readers_;
  std::unordered_map<FileId, std::jthread> file_reader_threads_;
  std::vector<FileReader> files;
  std::jthread event_processor_thread_;
  // Shared mutex for read/write operations on maps.
  std::shared_mutex rw_mutex_;
};
