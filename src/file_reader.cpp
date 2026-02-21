#include "file_reader.h"
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <sys/stat.h>
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
  file_fd = open_file(file_path_);
  if (file_fd == -1) {
    return;
  }

  // Jump to the end of the file.
  read_offset = jump_to_offset(file_fd, 0, SEEK_END);
  if (read_offset == -1) {
    cerr << format("Could not move to the end of file ", path_string_) << endl;
    return;
  }
  cout << format("Initial read offset {}", read_offset) << endl;

  // Inotify registration
  inotify_fd = register_with_inotify();
  if (inotify_fd == -1) {
    cerr << format("Could not move to the end of file ", path_string_) << endl;
    return;
  }

  // Set worker as 'alive'.
  is_alive_ = true;
}

int FileReader::open_file(const filesystem::path &file_path) {
  file_fd = open(file_path_.c_str(), O_RDONLY);
  if (file_fd == -1) {
    cerr << format("Failed to open file {}, {}", path_string_, strerror(errno))
         << endl;
  }

  // Set the last modified time
  optional<struct stat> file_stat{get_fstat(file_fd)};
  if (!file_stat.has_value()) {
    return -1;
  }

  last_mod_time = file_stat.value().st_mtime;
  return file_fd;
}

off_t FileReader::jump_to_offset(const int fd, const off_t offset,
                                 const int whence) {
  off_t final_offset = lseek(fd, offset, whence);
  if (final_offset == -1) {
    cerr << format("Failed to jump to offset {}", strerror(errno)) << endl;
  }
  return final_offset;
}

int FileReader::register_with_inotify() {
  // File descriptor for registration with inotify API so that
  // you only wakes up when any of the events in the mask happens.
  inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
  if (inotify_fd == -1) {
    // ToDo: Give a more detailed error message and check how to
    // propagate the error to the Manager.
    cerr << format("Failed to register inotify {}", strerror(errno)) << endl;
    return inotify_fd;
  }

  // Adding a watch for the created Inotify fd.
  if (inotify_add_watch(inotify_fd, file_path_.c_str(), mask) == -1) {
    cerr << format("Failed to add watch for inotify {}", strerror(errno))
         << endl;
    return -1;
  }
  return inotify_fd;
}

void FileReader::run() {
  ssize_t events_bytes_read;
  while (is_alive_) {
    // The following read() is NOT for the file that you want to read but
    // rather for the inotify list of events.
    events_bytes_read =
        read(inotify_fd, inotify_buf.data(), inotify_buf.size());

    // Error handling for inotify fd
    if (events_bytes_read <= 0) {
      if (events_bytes_read == -1 && errno != EAGAIN) {
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
    for (char *ptr = inotify_buf.data();
         ptr < inotify_buf.data() + events_bytes_read;
         ptr += sizeof(struct inotify_event) + event->len) {

      // This  is a delibrate conversion of char* to inotify event struct
      // pointer. It IS unsafe. Didn't find any other way at the time.
      event = reinterpret_cast<inotify_event *>(ptr);
      if ((event->mask & IN_MODIFY) != 0 && !is_modify_event_read) {
        status = handle_file_modify();
        is_modify_event_read = true;
      } else if (event->mask & IN_ATTRIB) {
        cout << "IN_ATTRIB event" << endl;
      } else if (event->mask & IN_MOVE_SELF) {
        cout << "IN_MOVE_SELF event" << endl;
      } else if (event->mask & IN_DELETE_SELF) {
        cout << "IN_DELETE_SELF event" << endl;
      } else {
        cout << format("Event mask {}, no implementation for this event yet.",
                       event->mask)
             << endl;
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

// optional<off_t> FileReader::get_file_size(int fd) {
//   struct stat buf;
//   if (fstat(fd, &buf) == -1) {
//     cerr << format("Failed to get stat for fd {}, {}", fd, strerror(errno));
//     return nullopt;
//   }

//   return buf.st_size;
// }

// optional<time_t> FileReader::get_last_modified_time(int fd) {
//   struct stat buf;
//   if (fstat(fd, &buf) == -1) {
//     cerr << format("Failed to get stat for fd {}, {}", fd, strerror(errno));
//     return nullopt;
//   }

//   return buf.st_mtime;
// }

optional<struct stat> FileReader::get_fstat(int fd) {
  struct stat buf;
  if (fstat(fd, &buf) == -1) {
    cerr << format("Failed to get stat for fd {}, {}", fd, strerror(errno));
    return nullopt;
  }

  return buf;
}

EventHandlerStatus FileReader::handle_file_modify() {
  cout << "File modified" << endl;
  // Check if file was truncated.
  optional<struct stat> file_stat{get_fstat(file_fd)};
  if (!file_stat.has_value()) {
    return EventHandlerStatus::ERROR;
  }

  off_t file_size = file_stat.value().st_size;
  time_t updated_last_mod_time = file_stat.value().st_mtime;

  cout << format("{}, {}, {}, {}", read_offset, file_size, last_mod_time,
                 updated_last_mod_time)
       << endl;

  // ToDo: Look for nanoseconds or inode based solution since any change faster
  // than 1 sec won't be registered by st_mtime.
  if (file_size < read_offset || updated_last_mod_time != last_mod_time) {
    // File was truncated. Reload the file.
    last_mod_time = updated_last_mod_time;
    if (auto status = handle_file_truncated();
        status != EventHandlerStatus::CONTINUE) {
      return status;
    };
  }

  int data_bytes_read = read(file_fd, file_buf.data(), file_buf.size());
  string_view data(file_buf.data(), data_bytes_read);
  cout << format("data from file {}", data) << endl;
  read_offset += data_bytes_read;
  return EventHandlerStatus::CONTINUE;
}

EventHandlerStatus FileReader::handle_file_truncated() {
  cout << "File truncated" << endl;
  off_t new_offset = jump_to_offset(file_fd, 0, SEEK_SET);
  if (new_offset == -1) {
    cout << "Failed to reset the read offset after truncation" << endl;
    return EventHandlerStatus::STOP;
  }

  // Reset the file's reading offset
  read_offset = new_offset;

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
