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
  std::atomic<bool> is_alive_ = false;

public:
  FileReader(std::filesystem::path file_path);
  bool is_alive();
  void run();
  void on_event_file_modify(int file_fd);
  void cleanup();
};
