#pragma once
#include <utils/types.h>

namespace Events {

struct DataAvailable {
  FileId id;
  std::string data;
};

struct FileError {
  FileId id;
  std::error_code error;
};

struct FileClosed {
  FileId id;
};
} // namespace Events
