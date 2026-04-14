#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
class UIManager {
public:
  // Setting the constructors.
  UIManager(UIManager &&other) = delete;            // Moving is allowed
  UIManager &operator=(UIManager &&other) = delete; // reassignment not allowed
  UIManager(const UIManager &) = delete;            // copying not allowed
  UIManager &
  operator=(const UIManager &) = delete; // copying reassignment not allowwed

  UIManager();
  void process_file_events();
  void run();

private:
  ftxui::Component compose();
  ftxui::Component root_container = ftxui::Container::Horizontal({});
  ftxui::ScreenInteractive screen_;
};
