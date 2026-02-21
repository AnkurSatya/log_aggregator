#pragma once
#include <array>
#include <atomic>
#include <filesystem>
#include <optional>
#include <sys/inotify.h>

enum class EventHandlerStatus {
  CONTINUE, // Everything is fine
  STOP,     // Stop processing events and exit gracefully
  ERROR,    // An unexpected error occur
};

class FileReader {
private:
  std::filesystem::path file_path_;
  std::string path_string_;
  // Mask for inotify events to be listend to.
  uint32_t mask = IN_MODIFY | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;
  // Useful for other threads to check if this thread is active or not.
  std::atomic<bool> is_alive_{false};
  off_t read_offset{0};
  time_t last_mod_time;
  int file_fd{-1};
  int inotify_fd{-1};
  std::array<char, 4096> inotify_buf;
  std::array<char, 4096> file_buf;

public:
  FileReader(std::filesystem::path file_path);
  int open_file(const std::filesystem::path &file_path);
  off_t jump_to_offset(const int fd, const off_t offset, const int whence);
  int register_with_inotify();
  std::optional<struct stat> get_fstat(int fd);
  // std::optional<off_t> get_file_size(int fd);
  // std::optional<time_t> get_last_modified_time(int fd);
  bool is_file_truncated(off_t current_offset, int fd);
  EventHandlerStatus handle_file_modify();
  EventHandlerStatus handle_file_truncated();
  void run();
  bool is_alive();
  bool stop();
  void cleanup();
};
