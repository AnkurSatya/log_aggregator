#pragma once
#include <core/events.h>
#include <filesystem>
#include <stop_token>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <utils/thread_safe_queue.h>
#include <utils/types.h>
#include <utils/unique_fd.h>

enum class EventHandlerStatus {
  CONTINUE, // Everything is fine
  STOP,     // Stop processing events and exit gracefully
  ERROR,    // An unexpected error occur
};

struct FileInfo {
  FileId file_id;
  std::filesystem::path file_path;
  unique_fd fd;
  struct stat file_stat;
  off_t read_offset;
  unique_fd inotify_fd;
};

class FileReader {
private:
  explicit FileReader(FileInfo file_info) noexcept;
  int fd() const noexcept { return file_info_.fd.get(); }

  // For data file ---
  FileInfo file_info_;
  std::string path_string_;

  // For Inotify ---
  // Mask for inotify events to be listend to.
  uint32_t mask_ = IN_ALL_EVENTS;

public:
  static constexpr size_t BUF_CHUNK_SIZE = 4096;
  ~FileReader() = default;
  // Setting the constructors.
  FileReader(FileReader &&other) noexcept = default; // Moving is allowed
  FileReader &
  operator=(FileReader &&other) = delete;  // reassignment not allowed
  FileReader(const FileReader &) = delete; // copying not allowed
  FileReader &
  operator=(const FileReader &) = delete; // copying reassignment not allowwed

  // static Result<FileReader, std::error_code>
  static Result<FileReader, std::error_code>
  open_file(const FileId file_id, const std::filesystem::path &file_path);
  static Result<struct stat, std::error_code> get_fstat(int fd);
  static Result<off_t, std::error_code> jump_to_offset(int fd, off_t offset,
                                                       int whence);
  static Result<struct stat, std::error_code>
  get_stat(const std::filesystem::path &filepath);
  static Result<unique_fd, std::error_code>
  register_with_inotify(uint32_t mask);

  static Result<int, std::error_code>
  add_inotify_file_watch(int inotify_fd, const std::filesystem::path &path,
                         uint32_t mask);

  static Result<int, std::error_code>
  add_inotify_dir_watch(int inotify_fd, const std::filesystem::path &dir,
                        uint32_t mask);

  Result<std::string, log_aggregator::Error> read_new_data();
  Result<bool, log_aggregator::Error> is_file_truncated();
  Result<std::string, log_aggregator::Error> handle_file_modify();
  Result<void, log_aggregator::Error> handle_file_truncated();
  Result<EventHandlerStatus, log_aggregator::Error>
  handle_file_attribute_changed();
  EventHandlerStatus handle_file_rotated();
  void run(std::stop_token, ThreadSafeQueue<FileProcessingEvent> &event_queue);
  static bool is_blank(std::string_view);
  void cleanup();
  void stop();
};
