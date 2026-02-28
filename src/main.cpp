#include "file_reader.h"
#include <format>
#include <iostream>

using namespace std;
int main() {
  filesystem::path file_path{"/home/ankur/projects/log_aggregator/app.log"};
  FileReader file_reader{file_path};

  if (file_reader.is_alive()) {
    file_reader.run();
  } else {
    cerr << format("Reader could not be setup for file {}", file_path.string())
         << endl;
  }
}
