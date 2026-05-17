#include <ftxui/component/event.hpp>
#include <ui/control_panel.h>

using namespace std;
using namespace ftxui;
using namespace spdlog;

ControlPanel::ControlPanel(shared_ptr<spdlog::logger> logger)
    : logger_{std::move(logger)} {
  compose();
}

void ControlPanel::compose() {
  root_container_->Add(compose_file_browser());
  // auto save_button auto close_button = Button("[x]", [this, file_id] {
  //   if (callback_pane_close_) {
  //     callback_pane_close_(file_id);
  //   }
  // });

  // // The first argument is the component that should receive events.
  // return Renderer(close_button, [this, close_button, file_id] {
  //   lock_guard lock(this->pane_update_mutex_);
  //   auto pane = get_pane(file_id);
  //   Elements data_rows;
  //   string heading;

  //   if (pane == nullptr) {
  //     data_rows.push_back(text("File no longer available"));
  //   } else {
  //     for (const auto &line : pane->data) {
  //       data_rows.push_back(text(line));
  //     }
  //     heading = pane->path.string();
  //   }

  //   auto header = hbox({text(heading), close_button->Render()});

  //   auto body = data_rows.empty() ? text("") | dim | center
  //                                 : vbox(data_rows) | yframe | flex;

  //   return vbox({header, separator(), body}) | border | flex;
  // });
}

Component ControlPanel::compose_file_browser() {
  return compose_searchable_menu();
}

Component ControlPanel::compose_searchable_menu() {
  auto files = make_shared<vector<string>>();
  auto user_text = make_shared<string>();

  // Component for taking user input
  InputOption input_options;
  input_options.on_change = [files, user_text, this]() {
    auto files_found = scan_directory(*user_text);
    if (files_found) {
      debug("Files found");
      *files = files_found.value();
    } else {
      debug("Files not found");
      *files = vector<string>{std::move(files_found.error().message)};
    }

    // Notify the ftxui::Screen to do a refresh
    if (on_data_changed_) {
      on_data_changed_();
    } else {
      spdlog::debug("No refresh trigget set.");
    }
  };

  auto input_field = Input(user_text.get(),
                           "Type to search and press spacebar to select a file",
                           input_options);

  // Component for displaying files available based on user input.
  MenuOption menu_options;
  // IMPORTANT: DO NOT pass *files as an argument since that would pass a
  // reference to the underlying vector and Menu has a constructor which when
  // receives this, creates a copy of the vector and hence any future changes in
  // the vector would not be reflected.
  menu_options.entries = files.get();
  menu_options.selected = 0;
  auto menu = Menu(menu_options);

  auto container = Container::Vertical({input_field, menu});

  input_field->TakeFocus();

  return Renderer(container, [menu, input_field] {
    return vbox({input_field->Render(), separator(), menu->Render() | frame}) |
           border;
  });

  // ToDo:
  // 1. Test rendering of Searchable_menu and test the rendering by typing a
  // path.
  // 2. Add keyboard events to menu, spacebar to select
}

Result<std::vector<string>, log_aggregator::Error>
ControlPanel::scan_directory(string_view raw_path) {
  filesystem::path path{raw_path};
  if (is_valid_directory(path)) {
    vector<string> files;
    for (auto const &dir_entry : filesystem::directory_iterator{path}) {
      files.push_back(std::move(dir_entry.path()));
    }
    return std::move(files);
  }

  return unexpected(log_aggregator::Error{
      make_error_code(errc::not_a_directory),
      format("Path is not a valid directory: {}", raw_path)});
}

bool ControlPanel::is_valid_directory(filesystem::path path) {
  return filesystem::exists(path) && filesystem::is_directory(path);
}
