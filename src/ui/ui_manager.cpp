#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ui/ui_manager.h>

using namespace std;
using namespace ftxui;

UIManager::UIManager() : screen_{ftxui::ScreenInteractive::Fullscreen()} {
  // auto screen = Screen::Create(Dimension::Fixed(32), Dimension::Fixed(10));

  // auto &pixel = screen.CellAt(9, 9);
  // pixel.character = U'A';
  // pixel.bold = true;
  // pixel.foreground_color = Color::Blue;

  // std::cout << screen.ToString() << endl;

  int counter = 0;
  auto on_click = [&] { counter++; };
  auto container = Container::Vertical({});
  auto button = Button("Button", on_click);
  container->Add(button);

  auto renderer = Renderer(container, [&] {
    return vbox({
               hbox({text("Counter: "), text(std::to_string(counter))}),
               separator(),
               container->Render() | vscroll_indicator | frame |
                   size(HEIGHT, LESS_THAN, 20),
           }) |
           border;
  });

  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(renderer);
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
