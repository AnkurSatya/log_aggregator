#pragma once
#include <string>
#include <system_error>
#include <utils/types.h>
#include <variant>

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

using FileProcessingEvent =
    std::variant<Events::DataAvailable, Events::FileError, Events::FileClosed>;
