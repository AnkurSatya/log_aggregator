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

  UIManager(FileManager &);
  void process_file_events();
  void run();

private:
  FileManager &file_manager_;
  Viewport viewport_;
  ftxui::ScreenInteractive screen_;

  void exit_application();
  ftxui::Component compose();
  ftxui::Component root_container = ftxui::Container::Horizontal({});
};
