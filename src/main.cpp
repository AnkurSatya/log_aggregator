#include "proto/log_aggregator/file_events.pb.h"
#include <core/file_manager.h>
#include <format>
#include <iostream>
#include <ui/ui_manager.h>
#include <utils/config.h>
#include <utils/messenger.h>

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
  // ToDo
  // 1. Create a UI class.
  // 2. Initialise it here and launch the UI::run() in a thread.
  // 3. Create a threadsafe queue to be shared between UI and FileManager.
  // 4. Push a DataAvailable event from FileManager with empty data to trigger
  // Tile creation for the file in the UI.

  schema::DataAvailable msg;
  msg.set_id(1);
  msg.set_data("dfff");
  std::string serialized_data;
  msg.SerializeToString(&serialized_data);

  auto shared_zmq_ctx{make_shared<zmq::context_t>()};
  ZmqSocketConfig socket_config{
      .sock_addr = "inproc://log_aggregator",
      .socket_type = zmq::socket_type::pair,
      .send_flags = zmq::send_flags::dontwait,
  };

  FileManager file_manager{shared_zmq_ctx, socket_config};

  UIManager ui_manager(file_manager, shared_zmq_ctx, socket_config);
  ui_manager.run();

  vector<std::string> file_paths = {
      "/home/ankur/projects/log_aggregator/app.log",
      "/home/ankur/projects/log_aggregator/app1.log"};

  for (const auto &path : file_paths) {
    Result<FileId, Error> file_id = file_manager.add_file(path);
    if (!file_id) {
      cerr << format("Failed to open file {}: {}, {}", path,
                     file_id.error().code.message(), file_id.error().message)
           << endl;
    }
  }

  file_manager.start_event_processing();
  // Temporary fix to let the threads run until user stops the application.
  unique_lock<mutex> lock(shutdown_mtx);
  shutdown_cv.wait(lock, [] { return shutdown_requested; });
  return 0;
}
