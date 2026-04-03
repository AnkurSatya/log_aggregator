#pragma once
#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

template <typename T, typename E> using Result = std::expected<T, E>;

using FileId = uint32_t;

namespace log_aggregator {
struct Error {
  std::error_code code;
  std::string message;

  static Error from_errno(int err_num, const std::string &context) {
    return {std::error_code(err_num, std::system_category()), context};
  }
};
} // namespace log_aggregator
