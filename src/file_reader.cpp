#include "file_reader.h"
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <sys/fanotify.h>
#include <thread>
#include <unistd.h>

using namespace std;

FileReader::FileReader(std::filesystem::path file_path)
    : file_path_(std::move(file_path)) {
  if (!filesystem::exists(file_path_)) {
    cerr << format("File {} does not exist", file_path.string()) << endl;
    return;
  }
  path_string_ = file_path_.string();

  // ToDo: Put somekind of timer to let the file be opened (if required)
  // Open file
  file_fd_ = open_file(file_path_);
  if (file_fd_ == -1) {
    return;
  }

  // Register with fanotify
  fanotify_fd_ = register_with_fanotify();
  if (fanotify_fd_ == -1) {
    cerr << format("Could not register with fanotify for file {}", path_string_)
         << endl;
    return;
  }

  // Jump to the end of the file.
  read_offset_ = jump_to_offset(file_fd_, 0, SEEK_END);
  if (read_offset_ == -1) {
    cerr << format("Could not move to the end of file {}", path_string_)
         << endl;
    return;
  }
  cout << format("Initial read offset {}", read_offset_) << endl;

  // Inotify registration
  inotify_fd_ = register_with_inotify();
  if (inotify_fd_ == -1) {
    cerr << format("Could not register with inotify for file {}", path_string_)
         << endl;
    return;
  }

  // Set worker as 'alive'.
  is_alive_ = true;
}

optional<struct stat> FileReader::get_fstat(int fd) {
  struct stat buf;
  if (fstat(fd, &buf) == -1) {
    cerr << format("Failed to get stat for fd {}, {}", fd, strerror(errno));
    return nullopt;
  }

  return buf;
}

int FileReader::open_file(const filesystem::path &file_path) {
  file_fd_ = open(file_path_.c_str(), O_RDONLY);
  if (file_fd_ == -1) {
    cerr << format("Failed to open file {}, {}", path_string_, strerror(errno))
         << endl;
  }

  // Set the last modified time
  optional<struct stat> current_file_stat{get_fstat(file_fd_)};
  if (!current_file_stat.has_value()) {
    return -1;
  }

  file_stat_ = current_file_stat.value();
  return file_fd_;
}

off_t FileReader::jump_to_offset(const int fd, const off_t offset,
                                 const int whence) {
  off_t final_offset = lseek(fd, offset, whence);
  if (final_offset == -1) {
    cerr << format("Failed to jump to offset {}", strerror(errno)) << endl;
  }
  return final_offset;
}

int FileReader::register_with_fanotify() {
  // FAN_CLASS_CONTENT - allows to intercept the file access before the file is
  // opened/read. This is useful to detect truncation.
  // FAN_NONBLOCK - non blocking read.
  int fd = fanotify_init(FAN_CLASS_CONTENT | FAN_NONBLOCK, O_RDWR);
  if (fd == -1) {
    // ToDo: Give a more detailed error message and check how to
    // propagate the error to the Manager.
    cerr << format("Failed to register fanotify, {}", strerror(errno)) << endl;
    return fd;
  }

  // Mark the file for open permission events(Meaning telling the OS that you
  // are interested in some specific events on this file)
  // FAN_MARK - to add a mark
  // FAN_OPEN_PERM - To inspect events like open() before they complete and then
  // tell OS whether it should be allowed or not. This could be useful for virus
  // scanning kind of stuff.
  // AT_FDCWD - to show that the path passed is relative to cwd but it is
  // ignored if the passed path is absolute. It is just a convention to put it
  // in that case.

  uint32_t mask = FAN_OPEN_PERM;
  // | FAN_MODIFY | FAN_CLOSE_WRITE;
  if (fanotify_mark(fd, FAN_MARK_ADD, mask, AT_FDCWD, path_string_.c_str())) {
    cerr << format("Failed to add mark for file at {}, {}", path_string_,
                   strerror(errno))
         << endl;
    return -1;
  }
  return fd;
}

int FileReader::register_with_inotify() {
  // File descriptor for registration with inotify API so that
  // you only wakes up when any of the events in the mask happens.
  int fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
  if (fd == -1) {
    // ToDo: Give a more detailed error message and check how to
    // propagate the error to the Manager.
    cerr << format("Failed to register inotify {}", strerror(errno)) << endl;
    return fd;
  }

  // Adding a watch for the created Inotify fd.
  if (inotify_add_watch(fd, file_path_.c_str(), mask_) == -1) {
    cerr << format("Failed to add watch for inotify {}", strerror(errno))
         << endl;
    return -1;
  }
  return fd;
}

// optional<bool> FileReader::is_file_truncated() {
//   ssize_t fanotify_bytes_read;
//   fanotify_bytes_read =
//       read(fanotify_fd_, fanotify_buf_.data(), fanotify_buf_.size());

//   if (fanotify_bytes_read == -1) {
//     return nullopt;
//   }

//   cout << format("FANOTIFY BYTES READ: {}", fanotify_bytes_read) << endl;
//   // Casting buffer to fanotify_event_metadata type.
//   fanotify_event_metadata *event =
//       reinterpret_cast<fanotify_event_metadata *>(fanotify_buf_.data());
//   while (FAN_EVENT_OK(event, fanotify_bytes_read)) {
//     // cout << format("Event mask: {}", event->mask) << endl;
//     if (event->mask & FAN_MODIFY) {
//       cout << "File Modified" << endl;
//       cout << "Size: " << get_fstat(file_fd_).value().st_size << endl;
//     }

//     if (event->mask & FAN_CLOSE_WRITE) {
//       cout << "File written" << endl;
//       cout << "Size: " << get_fstat(file_fd_).value().st_size << endl;
//     }

//     if (event->mask & FAN_OPEN_PERM) {
//       // int flags = fcntl(event->fd, F_GETFL);

//       // cout << format("Flags: {}", flags) << endl;
//       cout << "Open event" << endl;
//       cout << "Size before: " << get_fstat(file_fd_).value().st_size << endl;
//       // if (flags & O_TRUNC) {
//       //   cout << format("File at {} was truncated", path_string_) << endl;
//       // }

//       // Allow the file open to proceed.
//       fanotify_response response;
//       response.fd = event->fd;
//       response.response = FAN_ALLOW;
//       write(fanotify_fd_, &response, sizeof(response));

//       cout << "Size after: " << get_fstat(event->fd).value().st_size << endl;

//       close(event->fd);
//     }

//     if (event->fd >= 0) {
//       close(event->fd);
//     }

//     event = FAN_EVENT_NEXT(event, fanotify_bytes_read);
//   }
//   return false;
// }

void FileReader::run() {
  ssize_t inotify_bytes_read;
  while (is_alive_) {
    // Detect file truncation. It needs to be detected before processing Inotify
    // events since Inotify events does not know directly if a file was
    // truncated. And we use FANOTIFY events for this.
    optional<bool> file_truncated{is_file_truncated()};
    if (!file_truncated.has_value()) {
      this_thread::sleep_for(100ms);
      continue;
    }

    this_thread::sleep_for(500ms);
    continue;

    // The following read() is NOT for the file that you want to read but
    // rather for the inotify list of events.
    inotify_bytes_read =
        read(inotify_fd_, inotify_buf_.data(), inotify_buf_.size());

    // Error handling for inotify fd
    if (inotify_bytes_read <= 0) {
      if (inotify_bytes_read == -1 && errno != EAGAIN) {
        cerr << format("Failed to read file {}, {}", path_string_,
                       strerror(errno))
             << endl;
        is_alive_ = false;
        return;
      }

      if (errno == EOF) {
        // This should not happen while reading inotify events unless the FD has
        // been closed.
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
    bool is_modify_event_read{false};
    EventHandlerStatus status = EventHandlerStatus::CONTINUE;
    // sizeof (struct inotify event) would just the fix size of inotify
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
      if (event->mask & IN_MODIFY) {
        cout << "File modified" << endl;
        if (!is_modify_event_read) {
          status = handle_file_modify();
          is_modify_event_read = true;
        }
      }
      if (event->mask & IN_OPEN) {
        cout << "IN_OPEN event" << endl;
        cout << "Size after open: " << get_fstat(file_fd_).value().st_size
             << endl;
      }
      if (event->mask & IN_CLOSE_WRITE) {
        cout << "IN_CLOSE_WRITE event" << endl;
      }
      if (event->mask & IN_CLOSE_NOWRITE) {
        cout << "IN_CLOSE_NOWRITE event" << endl;
      }
      if (event->mask & IN_ATTRIB) {
        cout << "IN_ATTRIB event" << endl;
      }
      if (event->mask & IN_MOVE_SELF) {
        cout << "IN_MOVE_SELF event" << endl;
      }
      if (event->mask & IN_DELETE_SELF) {
        cout << "IN_DELETE_SELF event" << endl;
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

  // For IN_MODIFY look for a character like '/n' and then only add it to the
  // shared message queue.
}

optional<bool> FileReader::is_file_truncated() {
  optional<struct stat> updated_file_stat{get_fstat(file_fd_)};
  if (!updated_file_stat.has_value()) {
    return nullopt;
  }

  off_t updated_file_size = updated_file_stat.value().st_size;
  time_t updated_last_mod_time = updated_file_stat.value().st_mtim.tv_nsec;

  // If file size shrank, file was truncated.
  if (updated_file_size < file_stat_.st_size) {
    cout << "File shrank" << endl;
    return true;
  }

  // If file size stayed the same but the timestamp changed, file was
  // truncated or overwritten. This could happen if you do 'echo
  // <log_of_a_size> > log_file' again and again since the message size is
  // same but file is being overwritten.
  cout << format("{} {} {} {} {} {}", updated_file_size, file_stat_.st_size,
                 updated_file_stat->st_mtim.tv_sec, file_stat_.st_mtim.tv_sec,
                 updated_file_stat->st_mtim.tv_nsec, file_stat_.st_mtim.tv_nsec)
       << endl;
  cout << format("Old inode: {}, new inode: {}", file_stat_.st_ino,
                 updated_file_stat->st_ino)
       << endl;
  bool overwritten =
      (updated_file_size == file_stat_.st_size &&
       ((updated_file_stat->st_mtim.tv_sec != file_stat_.st_mtim.tv_sec) ||
        (updated_file_stat->st_mtim.tv_nsec != file_stat_.st_mtim.tv_nsec)));

  if (overwritten) {
    cout << "File overwritten" << endl;
    return true;
  }

  // If the new file size is greater but the cursor (read offset) of the new
  // file is less than the last saved read offset, then file was truncated.
  off_t new_file_offset = lseek(file_fd_, 0, SEEK_CUR);
  cout << format("New offset {}, old offset {}", new_file_offset, read_offset_)
       << endl;

  if (new_file_offset < read_offset_) {
    cout << "New file size greater than old. File truncated" << endl;
    return true;
  }
  return false;
}

EventHandlerStatus FileReader::handle_file_modify() {
  // cout << "File modified" << endl;
  // Check if file was truncated.
  optional<bool> file_truncated{is_file_truncated()};
  if (!file_truncated.has_value()) {
    return EventHandlerStatus::ERROR;
  }

  if (file_truncated.value()) {
    if (auto status = handle_file_truncated();
        status != EventHandlerStatus::CONTINUE) {
      return status;
    };
  }

  // ToDo: Read until '\n' or some other better end terminator.
  // ToDo: Implement for log rotation, may be use inode number for that.

  int data_bytes_read = read(file_fd_, file_buf_.data(), file_buf_.size());
  string_view data(file_buf_.data(), data_bytes_read);
  cout << format("data from file {}", data) << endl;

  // Update members related to file metadata
  read_offset_ += data_bytes_read;
  // Reset file stats
  if (auto current_file_stat{get_fstat(file_fd_)}) {
    file_stat_ = *current_file_stat;
  } else {
    return EventHandlerStatus::ERROR;
  }
  return EventHandlerStatus::CONTINUE;
  return EventHandlerStatus::CONTINUE;
}

EventHandlerStatus FileReader::handle_file_truncated() {
  off_t new_offset = jump_to_offset(file_fd_, 0, SEEK_SET);
  if (new_offset == -1) {
    cout << "Failed to reset the read offset after truncation" << endl;
    return EventHandlerStatus::STOP;
  }

  // Reset the file's reading offset
  read_offset_ = new_offset;
  return EventHandlerStatus::CONTINUE;
}

bool FileReader::is_alive() { return is_alive_.load(); }

bool FileReader::stop() {
  is_alive_ = false;
  // ToDo: Call cleanup and free other resources if necessary. See if this is
  // even needed in C++
  return true;
}

void FileReader::cleanup() {}

// ToDo: Rewrite processing of optional variables. Try to be less verbose.
//  This can be used: if(var) {process(*var)}
