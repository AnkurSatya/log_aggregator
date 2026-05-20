#include <ftxui/component/event.hpp>
#include <ui/control_panel.h>

using namespace std;
using namespace ftxui;
using namespace spdlog;

ControlPanelView::ControlPanelView(shared_ptr<spdlog::logger> logger)
    : logger_{std::move(logger)} {
  compose();
}

void ControlPanelView::compose() {
  auto searchable_menu = compose_searchable_menu();
  auto selected_files_panel = compose_selected_files_panel();

  auto container =
      Container::Horizontal({searchable_menu, selected_files_panel});

  auto renderer = Renderer(container, [searchable_menu, selected_files_panel] {
    return hbox({searchable_menu->Render(), separator(),
                 selected_files_panel->Render() | frame}) |
           border | flex;
  });
  root_container_->Add(renderer);
}

Component ControlPanelView::compose_searchable_menu() {
  // Rule of thumb while creating FTXUI components:
  // Provide a copy of the data that the component would be needing in order to:
  // 1. Remove its reliance on the lifetime of the class creating it like this
  // ControlPanelView class.
  // 2. No chances of dangling refrences then.

  // Creating new shared_ptrs so that the components created
  // can have their own copy and not depend on the lifetime of this class.
  auto state = model_;
  auto refresh_ui = on_data_changed_;

  // Component for taking user input
  InputOption input_options;
  input_options.on_change = [state, refresh_ui]() {
    state->update_file_search();
    if (refresh_ui)
      refresh_ui();
  };

  auto input_field =
      Input(&state->user_text,
            "Type to search and press Enter to select a file", input_options);

  // Component for displaying files available based on user input.
  MenuOption menu_options;
  menu_options.entries = &state->available_files;
  menu_options.selected = &state->menu_index;
  auto menu = Menu(menu_options);

  auto menu_container = Container::Vertical({input_field, menu});

  // Wrap the container in a Catch Event so as to be able to select files to be
  // added for tracking.
  auto menu_with_events =
      CatchEvent(menu_container, [state, refresh_ui](Event event) {
        if (event == Event::Return) {
          state->select_current_file();

          if (refresh_ui)
            refresh_ui();
          return true;
        }
        return false;
      });

  return Renderer(menu_with_events, [menu, input_field] {
    return vbox({input_field->Render(), separator(), menu->Render() | frame}) |
           border | flex;
  });
}

Component ControlPanelView::compose_selected_files_panel() {
  auto state = model_;
  MenuOption menu_options{.entries = &state->selected_files,
                          .selected = &state->selected_menu_index};
  auto menu = Menu(menu_options);

  return Renderer(menu, [state, menu] {
    auto header = hbox(text("Selected files"));
    return vbox({header, separator(), menu->Render() | frame}) | border | flex;
  });
}
