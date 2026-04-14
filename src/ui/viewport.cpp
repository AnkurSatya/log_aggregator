#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <mutex>
#include <ui/viewport.h>

using namespace std;
using namespace ftxui;

Viewport::Viewport() {
  // Create
}

Result<void, string> Viewport::add_pane(FileId file_id, filesystem::path path) {
  auto pane = FilePane{.file_id = file_id, .path{std::move(path)}, .data{}};
  if (!panes_.try_emplace(file_id, std::move(pane)).second) {
    return unexpected(
        format("A window for file at {} already exists.", path.string()));
  }
  return {};
}

void Viewport::remove_pane(FileId file_id) { panes_.erase(file_id); }

void Viewport::update_pane(FileId file_id, std::string new_data) {
  // Mutex is being used here because compose_pane() would also be accessing
  // FilePane.data
  lock_guard lock(pane_update_mutex_);
  auto it = panes_.find(file_id);
  if (it != panes_.end()) {
    it->second.data.push_back(new_data);
  }
}

Component Viewport::compose_pane(FilePane &pane) {
  auto on_click = [&] {};
  auto button = Button("Button", on_click);

  lock_guard lock(pane_update_mutex_);
  auto it = panes_.find(pane.file_id);
  Elements data_rows;
  if (it != panes_.end()) {
    for (const auto &line : it->second.data) {
      data_rows.push_back(text(line));
    }
  }
  return Renderer(button, [&, button] {
    return vbox({hbox({
                     text("") | bold | flex,
                     button->Render(),
                 }),
                 separator()}) |
           border | flex;
  });
}

Component Viewport::compose() {}

Element buildPaneGrid() {
  // This should call render on all the panes and this should be called by
  // UIManager.compose().
}
