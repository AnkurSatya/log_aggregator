#pragma once
#include <array>
#include <expected>
#include <filesystem>
#include <optional>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <utils/unique_fd.h>

enum class EventHandlerStatus {
  CONTINUE, // Everything is fine
  STOP,     // Stop processing events and exit gracefully
  ERROR,    // An unexpected error occur
};

struct FileInfo {
  std::filesystem::path file_path;
  unique_fd fd;
  struct stat file_stat;
  off_t read_offset;
};

class FileReader {
private:
  explicit FileReader(FileInfo file_info);
  int fd() const noexcept { return file_info_.fd.get(); }

  // For data file ---
  FileInfo file_info_;
  std::string path_string_;
  std::array<char, 4096> file_buf_;

  // For Inotify ---
  int inotify_fd_{-1};
  int inotify_file_watch_fd_{-1};
  int inotify_dir_watch_fd_{-1};
  std::array<char, 4096> inotify_buf_;
  // Mask for inotify events to be listend to.
  uint32_t mask_ = IN_ALL_EVENTS;
  // uint32_t mask_ = IN_MODIFY | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF |
  //                  IN_OPEN | IN_CLOSE_WRITE | IN_CREATE;

public:
  ~FileReader() = default;
  // Setting the constructors.
  FileReader(FileReader &&other) noexcept = default; // Moving is allowed

  FileReader &operator=(FileReader &&) = delete; // reassignment not allowed
  FileReader(const FileReader &) = delete;       // copying not allowed

  static std::expected<FileReader, std::error_code>
  open_file(std::filesystem::path file_path);

  static off_t jump_to_offset(const int fd, const off_t offset,
                              const int whence);
  static std::optional<struct stat> get_fstat(int fd);
  std::optional<struct stat> get_stat(const std::string &filepath);

  int register_with_inotify();
  int add_inotify_file_watch();
  int add_inotify_dir_watch();

  int read_new_data();
  std::optional<bool> is_file_truncated();
  EventHandlerStatus handle_file_modify();
  EventHandlerStatus handle_file_truncated();
  EventHandlerStatus handle_file_attribute_changed();
  EventHandlerStatus handle_file_rotated();
  void run();
  void cleanup();
  void stop();
};
