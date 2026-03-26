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
  uint32_t file_id = 0;
  filesystem::path file_path{"/home/ankur/projects/log_aggregator/app.log"};
  auto file_reader = FileReader::open_file(file_id, file_path);
  if (!file_reader)
    cerr << format("Failed to open file {}: {}", file_path.string(),
                   file_reader.error().message())
         << endl;

  // if (file_reader.is_alive()) {
  //   file_reader.run();
  // } else {
  //   cerr << format("Reader could not be setup for file {}",
  //   file_path.string())
  //        << endl;
  // }
}
