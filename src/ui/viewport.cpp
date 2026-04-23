#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <mutex>
#include <ui/viewport.h>

using namespace std;
using namespace ftxui;

Viewport::Viewport() {}
void Viewport::set_callback_pane_close(function<void(FileId)> callback) {
  callback_pane_close_ = std::move(callback);
}

Result<void, string> Viewport::add_pane(FileId file_id, filesystem::path path) {
  auto pane = FilePane{.file_id = file_id, .path{std::move(path)}, .data{}};
  if (!panes_.try_emplace(file_id, std::move(pane)).second) {
    return unexpected(
        format("Pane for {} is already registered.", path.string()));
  }

  // Add UI for the pane to the root container
  auto pane_view = compose_pane(file_id);
  if (!views_.try_emplace(file_id, pane_view).second) {
    return unexpected(
        format("A window for file at {} already exists.", path.string()));
  }
  root_container_->Add(pane_view);
  return {};
}

void Viewport::remove_pane(FileId file_id) {
  lock_guard lock(pane_update_mutex_);
  // First remove the view for the pane and then remove the associated FilePane
  // object since view has a reference for FilePane.
  auto it = views_.find(file_id);
  if (it != views_.end()) {
    it->second->Detach();
    views_.erase(file_id);
  }
  panes_.erase(file_id);
}

void Viewport::update_pane(FileId file_id, const string new_data) {
  // Mutex is being used here because compose_pane() would also be accessing
  // FilePane.data
  lock_guard lock(pane_update_mutex_);
  auto it = panes_.find(file_id);
  if (it != panes_.end()) {
    it->second.data.push_back(new_data);
  }
}

Component Viewport::compose_pane(FileId file_id) {
  auto close_button = Button("[x]", [this, file_id] {
    if (callback_pane_close_) {
      callback_pane_close_(file_id);
    }
  });

  // The first argument is the component that should receive events.
  return Renderer(close_button, [this, close_button, file_id] {
    Elements data_rows;
    auto pane = get_pane(file_id);
    if (pane != nullptr) {
      for (const auto &line : pane->data) {
        data_rows.push_back(text(line));
      }
    }

    auto header = hbox({text(pane->path.string()), close_button->Render()});

    auto body = data_rows.empty() ? text("") | dim | center
                                  : vbox(data_rows) | yframe | flex;

    return vbox({header, separator(), body}) | border | flex;
  });
}

const FilePane *Viewport::get_pane(FileId file_id) {
  lock_guard lock(pane_update_mutex_);
  auto it = panes_.find(file_id);
  if (it != panes_.end())
    return &(it->second);
  return nullptr;
}

// Component Viewport::compose() {
//   vector<Component> component_trees;
//   for (auto &pane : panes_) {
//     component_trees.push_back(compose_pane(pane.second));
//   }
//   // ToDo: Add the logic for checking max row and max col here, and create
//   the
//   // final container accordingly.
//   return Container::Horizontal(component_trees);
// }
