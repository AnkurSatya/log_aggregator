#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <ui/ui_manager.h>

using namespace std;
using namespace ftxui;

UIManager::UIManager(FileManager &file_manager)
    : file_manager_(file_manager),
      screen_{ftxui::ScreenInteractive::Fullscreen()} {
  // Set the callback for click on close button for a pane.
  viewport_.set_callback_pane_close([&](FileId file_id) {
    file_manager_.remove_file(file_id);
    viewport_.remove_pane(file_id);
    // Wakes up the render loop immediately
    screen_.Post(Event::Custom);
  });
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

void UIManager::run() {
  auto path = filesystem::path("a.log");
  auto result = viewport_.add_pane(1, path);
  viewport_.update_pane(1, "Test log 1");
  viewport_.update_pane(1, "Test log 2");
  viewport_.update_pane(1, "Test log 3");

  auto result2 = viewport_.add_pane(2, path);
  viewport_.update_pane(2, "Test log 4");
  viewport_.update_pane(2, "Test log 5");
  viewport_.update_pane(2, "Test log 6");
  if (!result) {
    cout << result.error() << endl;
    exit(1);
  }
  screen_.Loop(compose());
}

void UIManager::exit_application() { cout << "Exiting now ..." << endl; }
