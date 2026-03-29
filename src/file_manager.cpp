#include <file_manager.h>
#include <file_reader.h>

using namespace std;
Result<FileId> FileManager::add_file(const std::filesystem::path path) {
  filesystem::path file_path{path};
  FileId file_id = next_file_id_;
  auto file_reader = FileReader::open_file(file_id, file_path);
  if (!file_reader)
    return std::unexpected(file_reader.error());

  auto [_, insertion_success] =
      file_readers_.try_emplace(file_id, std::move(file_reader.value()));
  if (!insertion_success)
    return unexpected(error_code(EEXIST, std::system_category()));

  next_file_id_++;
  return next_file_id_;
}
