#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <ui/ui_manager.h>

using namespace std;
using namespace ftxui;
using namespace log_aggregator;

UIManager::UIManager(shared_ptr<zmq::context_t> ctx,
                     ZmqSocketConfig recv_socket_config,
                     shared_ptr<spdlog::logger> logger)
    : screen_{ftxui::ScreenInteractive::Fullscreen()},
      messenger_{ctx, recv_socket_config, recv_socket_callback(), logger},
      logger_{std::move(logger->clone("UIManager"))},
      viewport_(Viewport(logger)) {
  ctx_ = ctx;
  viewport_.set_callback_pane_close([&](FileId file_id) {
    viewport_.remove_pane(file_id);
    request_terminate_monitoring(file_id);
    // Wakes up the render loop immediately
    screen_.Post(Event::Custom);
  });
}

MessageCallback UIManager::recv_socket_callback() {
  return [this](const std::string &bytes) {
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
  };
}

void UIManager::setup_message_sender(ZmqSocketConfig send_socket_config) {
  messenger_.setup_sender(send_socket_config);
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
  // To trigger the sleeping UI loop.
  screen_.PostEvent(Event::Custom);
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

void UIManager::request_file_monitoring(FileId id, const string path) {
  // The following way of creating protobuf messages is preferred and safer
  // since envelope will have the ownership(allocated on the heap) and can be
  // easily moved.
  schema::FileService envelope;
  schema::FileCommands *command = envelope.mutable_file_commands();

  auto *add_file = command->mutable_add_file();
  add_file->set_id(id);
  add_file->set_path(std::move(path));

  messenger_.send(std::move(envelope));
}

void UIManager::request_terminate_monitoring(FileId id) {
  schema::FileService envelope;
  schema::FileCommands *command = envelope.mutable_file_commands();

  auto *close_file = command->mutable_close_file();
  close_file->set_id(id);

  messenger_.send(std::move(envelope));
}

void UIManager::run() {
  auto path = filesystem::path("/home/ankur/projects/log_aggregator/app.log");
  auto result = viewport_.add_pane(path);
  if (!result) {
    logger_->error(result.error());
    exit(1);
  }
  FileId file_id1 = result.value();
  request_file_monitoring(file_id1, path.string());

  auto path2 = filesystem::path("/home/ankur/projects/log_aggregator/app1.log");
  auto result2 = viewport_.add_pane(path2);
  if (!result2) {
    logger_->error(result2.error());
    exit(1);
  }
  FileId file_id2 = result2.value();
  request_file_monitoring(file_id2, path2.string());

  screen_.Loop(compose());
}

void UIManager::exit_application() {
  logger_->debug("UIManager exiting now ...");
}
