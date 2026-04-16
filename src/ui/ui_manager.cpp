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

  auto close_button = Button("[x]", [&] { exit_application(); });

  return Renderer(close_button, [&, close_button] {
    auto header = hbox({text("LOG AGGREGATOR"), close_button->Render()});
    auto body = vbox(viewport_root->Render()) | yframe | flex;
    return vbox({header, separator(), body}) | border | flex;
  });
}

void UIManager::run() { screen_.Loop(compose()); }

void UIManager::exit_application() { cout << "Exiting now ..." << endl; }
