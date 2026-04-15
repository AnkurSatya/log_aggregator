#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
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

  // int counter = 0;
  // auto on_click = [&] { counter++; };
  // auto container = Container::Vertical({});
  // auto button = Button("Button", on_click);
  // container->Add(button);

  // auto renderer = Renderer(container, [&] {
  //   return vbox({
  //              hbox({text("Counter: "), text(std::to_string(counter))}),
  //              separator(),
  //              container->Render() | vscroll_indicator | frame |
  //                  size(HEIGHT, LESS_THAN, 20),
  //          }) |
  //          border;
  // });

  // screen_.Loop(renderer);
}

Component UIManager::compose() {}

void UIManager::run() {
  // Consider adding a function called buildUI or something
  //  in UIManager which would call render on the return value of
  //  UIManager.compose(). UIManager.compose() should just create component
  //  tree from Viewport and ControlPanel by calling their compose() each, and
  //  buildUI() should render them and it should be passed to .loop().
  auto component_tree = compose();
  screen_.Loop(component_tree);
}
