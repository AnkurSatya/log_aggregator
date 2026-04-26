#pragma once
#include <string>
#include <system_error>
#include <utils/types.h>
#include <variant>

namespace native::Events {

struct DataAvailable {
  FileId id;
  std::string data;
};

struct FileError {
  FileId id;
  log_aggregator::Error error;
};

struct FileClosed {
  FileId id;
};

struct InotifyError {
  FileId id;
  std::error_code error_code;
};
} // namespace native::Events

using FileProcessingEvent =
    std::variant<native::Events::DataAvailable, native::Events::FileError,
                 native::Events::FileClosed, native::Events::InotifyError>;
