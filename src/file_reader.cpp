#include "file_reader.h"
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <unistd.h>
using namespace std;

FileReader::FileReader(std::filesystem::path file_path)
    : file_path_(std::move(file_path)) {
  if (!filesystem::exists(file_path_)) {
    cerr << format("File {} does not exist", file_path.string()) << endl;
    return;
  }
  path_string_ = file_path_.string();
  // Set worker as 'alive'.
  is_alive_ = true;
}

bool FileReader::is_alive() { return is_alive_.load(); }

void FileReader::run() {
  int inotify_fd, file_fd, watch_descriptor;
  ssize_t size;
  char buf[4096];

  // File descriptor for registration with inotify API so that
  // you only wakes up when any of the events in the mask happens.
  inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
  if (inotify_fd == -1) {
    // ToDo: Give a more detailed error message and check how to
    // propagate the error to the Manager.
    cerr << "inotify init1" << endl;
    is_alive_ = false;
    return;
  }

  // For watching Inotify events.
  watch_descriptor = inotify_add_watch(inotify_fd, file_path_.c_str(), mask);
  if (watch_descriptor == -1) {
    cerr << format("Cannot watch {}, {}", path_string_, strerror(errno))
         << endl;
    is_alive_ = false;
    return;
  }

  while (true) {
    size = read(inotify_fd, buf, sizeof(buf));
    if (size == -1 && errno != EAGAIN) {
      cerr << format("Failed to read file {}, {}", path_string_,
                     strerror(errno))
           << endl;
      is_alive_ = false;
      return;
    }

    // ToDo: Check how the following two errors link to the events in the mask.
    if (errno == EAGAIN) {
    }

    if (errno == EOF) {
    }

    for (char *ptr = buf; ptr < buf + size;) {
      // This  is a delibrate conversion of char* to inotify event struct
      // pointer. It IS unsafe.
      const struct inotify_event *event =
          reinterpret_cast<inotify_event *>(ptr);

      if ((event->mask & IN_MODIFY) != 0) {
      }
    }
  }

  // For IN_MODIFY look for a character like '/n' and then only add it to the
  // shared message queue.
  // Open when you need to actually read from the file.
  // file_fd = open(file_path_.c_str(), O_RDONLY);
}

void FileReader::on_event_file_modify(int file_fd) {
  cout << format("File modified, fd: {}", file_fd) << endl;
}

void FileReader::cleanup() { is_alive_ = false; }
