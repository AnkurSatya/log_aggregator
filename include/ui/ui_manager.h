#include "proto/log_aggregator/file_service.pb.h"
#include <core/file_manager.h>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ui/viewport.h>
class UIManager {
public:
  // Setting the constructors.
  UIManager(UIManager &&other) = delete;            // Moving is allowed
  UIManager &operator=(UIManager &&other) = delete; // reassignment not allowed
  UIManager(const UIManager &) = delete;            // copying not allowed
  UIManager &
  operator=(const UIManager &) = delete; // copying reassignment not allowwed

  UIManager(std::shared_ptr<zmq::context_t>, ZmqSocketConfig);
  void run();

private:
  Viewport viewport_;
  ftxui::Component root_container = ftxui::Container::Horizontal({});
  ftxui::ScreenInteractive screen_;
  Messenger messenger_;

  void request_file_monitoring(FileId, const std::string);
  void request_terminate_monitoring(FileId);
  void handle_file_events(log_aggregator::schema::FileEvents);
  ftxui::Component compose();
  void exit_application();
};
