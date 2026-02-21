#pragma once
// #include <errno.h>
#include <atomic>
#include <filesystem>
#include <sys/inotify.h>

class FileReader {
private:
  std::filesystem::path file_path_;
  std::string path_string_;
  // Mask for inotify events to be listend to.
  uint32_t mask = IN_MODIFY | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;
  // Useful for other threads to check if this thread is active or not.
  std::atomic<bool> is_alive_{false};
  int file_fd{-1};
  off_t read_offset{0};
  int inotify_fd{-1};

public:
  FileReader(std::filesystem::path file_path);
  int open_file(const std::filesystem::path &file_path);
  off_t jump_to_offset(const int fd, const off_t offset, const int whence);
  int register_with_inotify();
  void run();
  bool is_alive();
  void cleanup();
};
