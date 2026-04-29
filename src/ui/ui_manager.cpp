#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <ui/ui_manager.h>

using namespace std;
using namespace ftxui;
using namespace log_aggregator;

UIManager::UIManager(shared_ptr<zmq::context_t> ctx,
                     ZmqSocketConfig socket_config)
    : screen_{ftxui::ScreenInteractive::Fullscreen()},
      messenger_{ctx, socket_config.sock_addr, socket_config.socket_type,
                 socket_config.send_flags, socket_config.is_binder} {
  ctx_ = ctx;
  viewport_.set_callback_pane_close([&](FileId file_id) {
    viewport_.remove_pane(file_id);
    request_terminate_monitoring(file_id);
    // Wakes up the render loop immediately
    screen_.Post(Event::Custom);
  });
}

void UIManager::start_event_processing(ZmqSocketConfig recv_socket_config) {
  //  When you want to stop this thread, just destroy the zmq ctx. This means
  //  that the loop inside start_receiver() would break automatically when ctx
  //  is destroyed.
  messenger_.start_receiver(
      [this](const std::string &bytes) {
        schema::FileService envelope_event;
        if (envelope_event.ParseFromString(bytes)) {
          switch (envelope_event.payload_case()) {
          case schema::FileService::kFileEvents:
            this->handle_file_events(std::move(envelope_event.file_events()));
            break;
          case schema::FileService::kFileCommands:
            break;
          case schema::FileService::PAYLOAD_NOT_SET:
            cerr << "Unknown type of data received on ZMQ" << endl;
            break;
          }
        }
      },
      ctx_, recv_socket_config);
}

void UIManager::handle_file_events(
    log_aggregator::schema::FileEvents file_event) {
  switch (file_event.event_type_case()) {
  case schema::FileEvents::kDataAvailable: {
    viewport_.update_pane(file_event.data_available().id(),
                          std::move(file_event.data_available().data()));
    break;
  }
  case schema::FileEvents::kFileError: {
    viewport_.update_pane(file_event.file_error().id(),
                          std::move(file_event.file_error().error()));
    break;
  }
  case schema::FileEvents::kFileClosed: {
    viewport_.remove_pane(file_event.file_closed().id());
    break;
  }
  case schema::FileEvents::EVENT_TYPE_NOT_SET:
    cerr << "Unknown type of event received on ZMQ" << endl;
    break;
  }
}

Component UIManager::compose() {
  auto viewport_root = viewport_.get_root_container();

  auto close_button = Button("[x]", [this] { exit_application(); });

  // The first argument is the component that should receive events.
  // The second argument is the lambda and be careful about the types
  // (reference, ptr etc) of the parameters being passed to it. Consider the
  // possibility of dangling references.
  return Renderer(Container::Vertical({close_button, viewport_root}),
                  [close_button, viewport_root] {
                    auto header = hbox({text("LOG AGGREGATOR") | flex,
                                        close_button->Render()});
                    auto body = vbox({viewport_root->Render() | flex});
                    return vbox({header, separator(), body}) | border | flex;
                  });
}

void UIManager::request_file_monitoring(FileId, const string path) {
  schema::FileCommands::AddFile msg;
  msg.set_id(1);
  msg.set_path(std::move(path));
  messenger_.send(msg);
}

void UIManager::request_terminate_monitoring(FileId id) {
  schema::FileCommands::CloseFile msg;
  msg.set_id(1);
  messenger_.send(msg);
}

void UIManager::run() {
  auto path = filesystem::path("/home/ankur/projects/log_aggregator/app.log");
  auto result = viewport_.add_pane(path);
  if (!result) {
    cout << result.error() << endl;
    exit(1);
  }
  FileId file_id1 = result.value();
  request_file_monitoring(file_id1, path.string());

  // viewport_.update_pane(file_id1, "Test log 1");
  // viewport_.update_pane(file_id1, "Test log 2");
  // viewport_.update_pane(file_id1, "Test log 3");

  auto path2 = filesystem::path("/home/ankur/projects/log_aggregator/app1.log");
  auto result2 = viewport_.add_pane(path2);
  if (!result2) {
    cout << result.error() << endl;
    exit(1);
  }
  FileId file_id2 = result2.value();
  request_file_monitoring(file_id2, path.string());
  // viewport_.update_pane(file_id2, "Test log 4");
  // viewport_.update_pane(file_id2, "Test log 5");
  // viewport_.update_pane(file_id2, "Test log 6");

  screen_.Loop(compose());
}

void UIManager::exit_application() { cout << "Exiting now ..." << endl; }
