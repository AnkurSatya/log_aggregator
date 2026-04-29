#include <core/file_manager.h>
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
  auto shared_zmq_ctx{make_shared<zmq::context_t>()};

  ZmqSocketConfig core_socket_cfg{
      .sock_addr = "inproc://log_aggregator.core",
      .socket_type = zmq::socket_type::pair,
      .send_flags = zmq::send_flags::dontwait,
      .is_binder = true,
  };
  FileManager file_manager{shared_zmq_ctx, core_socket_cfg};
  cout << "FileManager successfully instantiated" << endl;

  ZmqSocketConfig ui_socket_cfg{
      .sock_addr = "inproc://log_aggregator.ui",
      .socket_type = zmq::socket_type::pair,
      .send_flags = zmq::send_flags::dontwait,
      .is_binder = true,
  };
  UIManager ui_manager(shared_zmq_ctx, ui_socket_cfg);
  cout << "UIManager successfully instantiated" << endl;

  file_manager.start_event_processing(ui_socket_cfg);
  cout << "File manager setup done" << endl;
  ui_manager.start_event_processing(core_socket_cfg);
  ui_manager.run();
  cout << "UI manager setup done" << endl;

  // Temporary fix to let the threads run until user stops the application.
  unique_lock<mutex> lock(shutdown_mtx);
  shutdown_cv.wait(lock, [] { return shutdown_requested; });
  return 0;
}
