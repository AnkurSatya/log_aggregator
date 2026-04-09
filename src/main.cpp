#include <file_manager.h>
#include <file_reader.h>
#include <format>
#include <iostream>

using namespace std;
using namespace log_aggregator;

condition_variable shutdown_cv;
mutex shutdown_mtx;
bool shutdown_requested = false;

void signal_handler(int signal) {
  {
    std::lock_guard<mutex> lock(shutdown_mtx);
    shutdown_requested = true;
  }
  // Wake up the main thread
  shutdown_cv.notify_all();
}

int main() {
  filesystem::path file_path{"/home/ankur/projects/log_aggregator/app.log"};
  FileManager file_manager;

  Result<FileId, Error> file_id = file_manager.add_file(file_path);
  if (!file_id) {
    cerr << format("Failed to open file {}: {}, {}", file_path.string(),
                   file_id.error().code.message(), file_id.error().message)
         << endl;
    return 1;
  }

  file_manager.start_event_processing();

  this_thread::sleep_for(1000ms);
  // file_manager.remove_file(file_id.value());
  // Temporary fix to let the threads run until user stops the application.
  unique_lock<mutex> lock(shutdown_mtx);
  shutdown_cv.wait(lock, [] { return shutdown_requested; });
  return 0;
}
