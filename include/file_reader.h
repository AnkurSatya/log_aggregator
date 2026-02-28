#pragma once
#include <array>
#include <atomic>
#include <filesystem>
#include <optional>
#include <sys/inotify.h>
#include <sys/stat.h>

enum class EventHandlerStatus {
  CONTINUE, // Everything is fine
  STOP,     // Stop processing events and exit gracefully
  ERROR,    // An unexpected error occur
};

class FileReader {
private:
  // For data file ---
  std::filesystem::path file_path_;
  std::string path_string_;
  struct stat file_stat_;
  int file_fd_{-1};
  off_t read_offset_{0};
  std::array<char, 4096> file_buf_;

  // For Fanotify ---
  int fanotify_fd_;
  std::array<char, 4096> fanotify_buf_;

  // For Inotify ---
  int inotify_fd_{-1};
  std::array<char, 4096> inotify_buf_;
  // Mask for inotify events to be listend to.
  uint32_t mask_ = IN_ALL_EVENTS;
  // uint32_t mask_ =
  //     IN_MODIFY | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF | IN_OPEN;

  // Useful for other threads to check if this thread is active or not.
  std::atomic<bool> is_alive_{false};

public:
  FileReader(std::filesystem::path file_path);
  int open_file(const std::filesystem::path &file_path);
  off_t jump_to_offset(const int fd, const off_t offset, const int whence);
  std::optional<struct stat> get_fstat(int fd);
  int register_with_fanotify();

  int register_with_inotify();
  std::optional<bool> is_file_truncated();
  EventHandlerStatus handle_file_modify();
  EventHandlerStatus handle_file_truncated();
  void run();
  bool is_alive();
  bool stop();
  void cleanup();
};
