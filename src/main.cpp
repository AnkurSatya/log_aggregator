#include <core/file_manager.h>
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

// ToDo:
//  1. Think more about the send flags:
//  a. Either they should be none but this might lead to a thread being stuck.
// b. Or it should be dontwait but then proper care should be taken in the
// beginning to ensure that clients are connected to the server before
// server(like UIManager) sends a message to the client(FileManager) and same
// goes for FIleManager -> UIManager.

int main() {
  auto shared_zmq_ctx{make_shared<zmq::context_t>()};

  ZmqSocketConfig core_socket_cfg{
      .socket_addr = "inproc://log_aggregator.core",
      .socket_type = zmq::socket_type::pair,
      .send_flags = zmq::send_flags::none,
  };
  FileManager file_manager{shared_zmq_ctx, core_socket_cfg};

  ZmqSocketConfig ui_socket_cfg{
      .socket_addr = "inproc://log_aggregator.ui",
      .socket_type = zmq::socket_type::pair,
      .send_flags = zmq::send_flags::none,
  };
  UIManager ui_manager(shared_zmq_ctx, ui_socket_cfg);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  file_manager.setup_message_sender(ui_socket_cfg);
  ui_manager.setup_message_sender(core_socket_cfg);
  ui_manager.run();

  // Temporary fix to let the threads run until user stops the application.
  unique_lock<mutex> lock(shutdown_mtx);
  shutdown_cv.wait(lock, [] { return shutdown_requested; });
  return 0;
}
