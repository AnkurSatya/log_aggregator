#pragma once
#include <unistd.h>
class unique_fd {
public:
  explicit unique_fd(int fd) noexcept : fd_(fd) {}

  ~unique_fd() {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  // move constructor - for transferring ownership from other to this
  unique_fd(unique_fd &&other) noexcept : fd_(other.fd_) {
    other.fd_ = -1; // source no longer owns fd.
  }

  unique_fd &operator=(unique_fd &&other) noexcept {
    if (this != &other) {
      if (fd_ != -1) {
        close(fd_);
      }
      fd_ = other.fd_;
      other.fd_ = -1;
    }
    return *this;
  }

  // no copying is allowed since one fd can't have two owners.
  unique_fd(const unique_fd &other) = delete;
  unique_fd &operator=(const unique_fd &other) = delete;

  int get() const noexcept { return fd_; }
  bool valid() const noexcept { return fd_ != -1; }
  int release() noexcept {
    int fd = fd_;
    fd_ = -1;
    return fd;
  }

private:
  int fd_ = -1;
};
