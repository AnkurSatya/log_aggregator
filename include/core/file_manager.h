#pragma once
#include "proto/log_aggregator/file_service.pb.h"
#include <core/events.h>
#include <core/file_reader.h>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <thread>
#include <unordered_map>
#include <utils/config.h>
#include <utils/messenger.h>
#include <utils/thread_safe_queue.h>

class FileManager {
public:
  FileManager(std::shared_ptr<zmq::context_t>, ZmqSocketConfig,
              std::shared_ptr<spdlog::logger>);
  void setup_message_sender(ZmqSocketConfig);

private:
  // Shared mutex for read/write operations on maps.
  std::shared_mutex rw_mutex_;
  Messenger messenger_;
  std::shared_ptr<zmq::context_t> ctx_;
  ThreadSafeQueue<FileProcessingEvent> event_queue_;
  std::jthread event_processor_thread_;
  // Storing shared_ptr instead of the object itself because it is required
  // independently at two places: in process_file() and remove_file().
  std::unordered_map<FileId, std::shared_ptr<FileReader>> file_readers_;
  std::unordered_map<FileId, std::jthread> file_reader_threads_;
  // detached but tracked threads.
  std::vector<std::jthread> orphaned_threads;
  std::shared_ptr<spdlog::logger> logger_;

  Result<void, log_aggregator ::Error> add_file(FileId file_id,
                                                const std::filesystem::path &);
  void process_file(std::stop_token, FileId id);
  void process_events(std::stop_token);
  void remove_file(FileId);
  void handle_file_commands(log_aggregator::schema::FileCommands);
  void handle(const native::Events::InotifyError &);
  void handle(const native::Events::FileError &);
  void handle(const native::Events::FileClosed &);
  void handle(const native::Events::DataAvailable &);
  void report_data_available(FileId, const std::string);
  void report_file_error(FileId, const std::string);
  void report_file_closed(FileId);
  MessageCallback recv_socket_callback();
};
