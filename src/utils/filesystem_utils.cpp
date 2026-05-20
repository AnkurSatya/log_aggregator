#include <format>
#include <utils/filesystem_utils.h>

namespace file_utils {
bool is_valid_directory(std::filesystem::path path) {
  return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

Result<std::vector<std::string>, log_aggregator::Error>
scan_directory(std::string_view raw_path) {
  std::filesystem::path path{raw_path};
  if (is_valid_directory(path)) {
    std::vector<std::string> files;
    for (auto const &dir_entry : std::filesystem::directory_iterator{path}) {
      files.push_back(std::move(dir_entry.path()));
    }
    return std::move(files);
  }

  return std::unexpected(log_aggregator::Error{
      make_error_code(std::errc::not_a_directory),
      format("Path is not a valid directory: {}", raw_path)});
}
} // namespace file_utils
