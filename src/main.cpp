#include "file_reader.h"
#include <iostream>

using namespace std;
int main() {
  filesystem::path file_path{"/home/ankur/projects/log_aggregator/test.log1"};
  FileReader file_reader{file_path};

  cout << file_reader.is_alive() << endl;
}
