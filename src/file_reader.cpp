#include "file_reader.h"
#include <array>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
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
  array<char, 4096> inotify_buf;
  array<char, 4096> file_buf;
  while (true) {
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
    // sizeof (struct inotify event) would just the fix size of inotify event
    // which does contain a field called "name" but it is empty in the struct
    // definition. When an actual event is read, it puts the data in
    // event->name and hence we need to increment by fixed struct size and
    // event->len(this gives the length of this additional data including
    // nuls)
    for (char *ptr = inotify_buf.data();
         ptr < inotify_buf.data() + events_bytes_read;
         ptr += sizeof(struct inotify_event) + event->len) {

      // This  is a delibrate conversion of char* to inotify event struct
      // pointer. It IS unsafe. Didn't find any other way at the time.
      event = reinterpret_cast<inotify_event *>(ptr);
      if ((event->mask & IN_MODIFY) != 0) {
        cout << "File modified" << endl;
        int data_bytes_read = read(file_fd, file_buf.data(), file_buf.size());
        string_view data(file_buf.data(), data_bytes_read);
        cout << format("data from file {}", data) << endl;
      }
    }

    this_thread::sleep_for(100ms);
  }

  // For IN_MODIFY look for a character like '/n' and then only add it to the
  // shared message queue.
}

bool FileReader::is_alive() { return is_alive_.load(); }

void FileReader::cleanup() { is_alive_ = false; }
