#include <file_manager.h>
#include <file_reader.h>
#include <format>
#include <iostream>

using namespace std;

void log_exception(const exception &e) {
  cerr << e.what() << endl;
  try {
    rethrow_if_nested(e);
  } catch (const exception &inner) {
    log_exception(inner);
  } catch (...) {
    cerr << "Unknown exception" << endl;
  }
}

int main() {
  filesystem::path file_path{"/home/ankur/projects/log_aggregator/app.log"};
  FileManager file_manager;
  Result<FileId> file_id = file_manager.add_file(file_path);
  if (!file_id)
    cerr << format("Failed to open file {}: {}", file_path.string(),
                   file_id.error().message())
         << endl;
}
