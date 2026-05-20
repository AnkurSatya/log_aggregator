#include <filesystem>
#include <utils/types.h>
#include <vector>

namespace file_utils {
bool is_valid_directory(std::filesystem::path path);
Result<std::vector<std::string>, log_aggregator::Error>
scan_directory(std::string_view raw_path);
} // namespace file_utils
