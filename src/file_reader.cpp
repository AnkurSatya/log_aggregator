#include "file_reader.h"
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;

FileReader::FileReader(FileInfo file_info) : file_info_(std::move(file_info)) {
  path_string_ = file_info_.file_path.string();
}

expected<FileReader, error_code>
FileReader::open_file(std::filesystem::path file_path) {
  if (!filesystem::exists(file_path)) {
    return unexpected(make_error_code(errc::no_such_file_or_directory));
  }

  unique_fd fd{open(file_path.c_str(), O_RDONLY)};
  if (fd.get() == -1) {
    return unexpected(error_code(errno, system_category()));
  }

  // ToDo: Move setup() inside this function and read more about factory pattern
  // logic.
  // Set the file stats
  optional<struct stat> new_file_stat{get_fstat(fd.get())};
  if (!new_file_stat) {
    return unexpected(error_code(errno, system_category()));
  }
  struct stat stat_ = new_file_stat.value();

  // Jump to the end of the file.
  off_t read_offset = jump_to_offset(fd.get(), 0, SEEK_END);
  if (read_offset == -1) {
    cerr << format("Could not move to the end of file {}", file_path.string())
         << endl;
    return unexpected(error_code(errno, system_category()));
  }
  cout << format("Initial read offset {}", read_offset) << endl;

  // // Inotify registration
  // if (register_with_inotify() == -1 || add_inotify_file_watch() == -1 ||
  //     add_inotify_dir_watch() == -1) {
  // }

  FileInfo info{.file_path = std::move(file_path),
                .fd = std::move(fd),
                .file_stat = std::move(stat_),
                .read_offset = 0};

  return FileReader(std::move(info));
}

optional<struct stat> FileReader::get_fstat(int fd) {
  struct stat buf;
  if (fstat(fd, &buf) == -1) {
    cerr << format("Failed to get stat for fd {}, {}", fd, strerror(errno));
    return nullopt;
  }

  return buf;
}

optional<struct stat> FileReader::get_stat(const string &filepath) {
  struct stat buf;
  if (stat(filepath.c_str(), &buf) == -1) {
    cerr << format("Failed to get stat for file {}, {}", filepath,
                   strerror(errno));
    return nullopt;
  }
  return buf;
}

off_t FileReader::jump_to_offset(const int fd, const off_t offset,
                                 const int whence) {
  off_t final_offset{lseek(fd, offset, whence)};
  if (final_offset == -1) {
    cerr << format("Failed to jump to offset {}", strerror(errno)) << endl;
  }
  return final_offset;
}

int FileReader::register_with_inotify() {
  // File descriptor for registration with inotify API so that
  // you only wakes up when any of the events in the mask happens.

  // Close any existing inotify
  if (inotify_fd_ != -1 && close(inotify_fd_) == -1) {
    cerr << format("Could not close existing Inotfiy fd, {}", strerror(errno))
         << endl;
    return -1;
  }

  inotify_fd_ = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
  if (inotify_fd_ == -1) {
    cerr << format("Failed to register inotify {}", strerror(errno)) << endl;
  }
  return inotify_fd_;
}

int FileReader::add_inotify_dir_watch() {
  // Add a watch on directory. This would be used for log rotation where the
  // existing file is moved and then re-created.

  string parent_dir = file_info_.file_path.parent_path();
  // Close any existing watch
  if (inotify_dir_watch_fd_ != -1 &&
      inotify_rm_watch(inotify_fd_, inotify_dir_watch_fd_) == -1) {
    cerr << format("Could not remove watch from directory {}, {}", parent_dir,
                   strerror(errno))
         << endl;
    return -1;
  }

  inotify_dir_watch_fd_ =
      inotify_add_watch(inotify_fd_, parent_dir.c_str(), IN_CREATE);
  if (inotify_dir_watch_fd_ == -1) {
    cerr << format("Failed to add Inotify directory watch for {}, {}",
                   parent_dir, strerror(errno))
         << endl;
  }
  return inotify_dir_watch_fd_;
}

int FileReader::add_inotify_file_watch() {
  if (inotify_file_watch_fd_ != -1 &&
      inotify_rm_watch(inotify_fd_, inotify_file_watch_fd_) == -1) {
    cerr << format("Could not remove watch from file{}, {}", path_string_,
                   strerror(errno))
         << endl;
    return -1;
  }

  inotify_file_watch_fd_ =
      inotify_add_watch(inotify_fd_, file_info_.file_path.c_str(), mask_);
  if (inotify_file_watch_fd_ == -1) {
    cerr << format("Failed to add Inotify file watch for {}, {}", path_string_,
                   strerror(errno))
         << endl;
  }
  return inotify_file_watch_fd_;
}

int FileReader::read_new_data() {
  // Update file stat
  if (auto current_file_stat{get_fstat(fd())}) {
    file_info_.file_stat = current_file_stat.value();
  } else {
    return -1;
  }
  size_t bytes_to_read = file_info_.file_stat.st_size - file_info_.read_offset;
  // Read and process the data.
  int data_bytes_read =
      pread(fd(), file_buf_.data(), bytes_to_read, file_info_.read_offset);
  if (data_bytes_read < 0) {
    return data_bytes_read;
  }

  string_view data(file_buf_.data(), data_bytes_read);
  cout << format("data from file {}", data) << endl;

  // Update members related to file metadata
  file_info_.read_offset += data_bytes_read - 1;

  return data_bytes_read;
}

optional<bool> FileReader::is_file_truncated() {
  optional<struct stat> updated_file_stat{get_fstat(fd())};
  if (!updated_file_stat) {
    return nullopt;
  }

  if (updated_file_stat->st_size < file_info_.read_offset) {
    cout << "File truncated" << endl;
    return true;
  }

  off_t updated_file_size{updated_file_stat.value().st_size};
  time_t updated_last_mod_time{updated_file_stat.value().st_mtim.tv_nsec};

  bool overwritten = (updated_file_size == file_info_.file_stat.st_size &&
                      ((updated_file_stat->st_mtim.tv_sec !=
                        file_info_.file_stat.st_mtim.tv_sec) ||
                       (updated_file_stat->st_mtim.tv_nsec !=
                        file_info_.file_stat.st_mtim.tv_nsec)));

  if (overwritten) {
    cout << "File overwritten" << endl;
    return true;
  }
  return false;
}

EventHandlerStatus FileReader::handle_file_truncated() {
  optional<bool> file_truncated{is_file_truncated()};
  if (!file_truncated) {
    return EventHandlerStatus::ERROR;
  }

  if (!file_truncated.value()) {
    return EventHandlerStatus::CONTINUE;
  }

  off_t new_offset{jump_to_offset(fd(), 0, SEEK_SET)};
  if (new_offset == -1) {
    cerr << "Failed to reset the read offset after truncation" << endl;
    return EventHandlerStatus::STOP;
  }

  // Reset the file's reading offset
  file_info_.read_offset = new_offset;
  return EventHandlerStatus::CONTINUE;
}

EventHandlerStatus FileReader::handle_file_modify() {
  // File truncation check.
  if (auto status = handle_file_truncated();
      status != EventHandlerStatus::CONTINUE) {
    return status;
  };

  if (read_new_data() == -1) {
    return EventHandlerStatus::ERROR;
  }
  return EventHandlerStatus::CONTINUE;
}

EventHandlerStatus FileReader::handle_file_attribute_changed() {
  cout << "File attributes changed" << endl;
  optional<struct stat> updated_file_stat{get_fstat(fd())};
  if (!updated_file_stat) {
    return EventHandlerStatus::ERROR;
  }

  // st_nlink is set to zero but the inode is still alive because of two
  // reasons: 1. Inotify watch on the fd is still active; 2. Fd is still open
  // This is why delete event is being caught by attribute change.
  if (updated_file_stat.value().st_nlink == 0) {
    cout << "File has been deleted" << endl;
    return EventHandlerStatus::STOP;
  }
  return EventHandlerStatus::CONTINUE;
}

// EventHandlerStatus FileReader::handle_file_rotated() {
//   // Reopen the file
//   if (fd() != -1) {
//     close(fd());
//   }
//   file_fd_ = open_file(file_path_);
//   if (file_fd_ == -1) {
//     return EventHandlerStatus::STOP;
//   }

//   // Reset the Inotify file watch
//   if (add_inotify_file_watch() == -1) {
//     return EventHandlerStatus::STOP;
//   }

//   // Read all the data from the reopened file
//   read_offset_ = 0;
//   if (read_new_data() == -1) {
//     return EventHandlerStatus::STOP;
//   }
//   return EventHandlerStatus::CONTINUE;
// }

void FileReader::run() {
  ssize_t inotify_bytes_read;
  // while (is_alive_) {
  while (1) {
    // The following read() is for the file that reads Inotify events.
    inotify_bytes_read =
        read(inotify_fd_, inotify_buf_.data(), inotify_buf_.size());

    // Error handling for inotify fd
    if (inotify_bytes_read <= 0) {
      if (inotify_bytes_read == -1 && errno != EAGAIN) {
        cerr << format("Failed to read file {}, {}", path_string_,
                       strerror(errno))
             << endl;
        // is_alive_ = false;
        return;
      }

      if (errno == EOF) {
        // This should not happen while reading inotify events unless the
        // Inotify FD has been closed.
        cerr << format("Inotify file descriptor closed for file {}, {}",
                       path_string_, strerror(EOF))
             << endl;
        return;
      }

      if (errno == EAGAIN) {
        // Not an error. It just means there are no inotify events right now.
        this_thread::sleep_for(100ms);
        continue;
      }
    }

    // Processing inotify events
    struct inotify_event *event;
    EventHandlerStatus status{EventHandlerStatus::CONTINUE};
    // sizeof (struct inotify event) would just give the fix size of inotify
    // event which does contain a field called "name" but it is empty in the
    // struct definition. When an actual event is read, it puts the data in
    // event->name and hence we need to increment by fixed struct size and
    // event->len(this gives the length of this additional data including
    // nuls)
    for (char *ptr = inotify_buf_.data();
         ptr < inotify_buf_.data() + inotify_bytes_read;
         ptr += sizeof(struct inotify_event) + event->len) {

      // This  is a delibrate conversion of char* to inotify event struct
      // pointer. It IS unsafe. Didn't find any other way at the time.
      event = reinterpret_cast<inotify_event *>(ptr);

      if (event->mask & IN_MODIFY || event->mask & IN_CLOSE_WRITE) {
        status = handle_file_modify();
      }

      if (event->mask & IN_CREATE) {
        if (event->name == file_info_.file_path.filename()) {
          cout << format("File {} created",
                         file_info_.file_path.filename().string())
               << endl;
          // status = handle_file_rotated();
        }
      }

      if (event->mask & IN_MOVE_SELF) {
        cout << "IN_MOVE_SELF event" << endl;
      }

      // For detecting if file has been deleted.
      if (event->mask & IN_ATTRIB) {
        if (auto status = handle_file_attribute_changed();
            status != EventHandlerStatus::CONTINUE) {
          return;
        }
      }
      if (event->mask & IN_DELETE || event->mask & IN_DELETE_SELF) {
        cout << "File deleted. Exiting ..." << endl;
        return;
      }

      // Process the status
      if (status == EventHandlerStatus::ERROR) {
        // ToDo: May be retry something here instead of exiting.
        return;
      } else if (status == EventHandlerStatus::STOP) {
        return;
      }
    }

    this_thread::sleep_for(100ms);
  }
}

// void FileReader::cleanup() {
//   if (file_fd_ != -1) {
//     close(file_fd_);
//   }

//   if (inotify_fd_ != -1) {
//     close(inotify_fd_);
//   }
// }

// FileReader::~FileReader() { cleanup(); }

// ToDo: Use RAII wrapper class for file descriptors which would automatically
// close the fds on exception or exit.
