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

Component ControlPanel::compose_searchable_menu() {
  // Component for taking user input
  InputOption input_options;
  // The parameter "user_text" passed needs to outlive this function itself
  // since on_change creates a copy of the v
  input_options.on_change = [this]() {
    if (user_text_->empty()) {
      available_files_->clear();
      *menu_index_ = 0;
      if (on_data_changed_) {
        on_data_changed_();
      }
      return;
    }

    auto files_found = scan_directory(user_text_ ? *user_text_ : "");
    if (files_found) {
      debug("Files found");
      *available_files_ = files_found.value();
    } else {
      debug("Files not found");
      *available_files_ =
          vector<string>{std::move(files_found.error().message)};
    }

    // Notify the ftxui::Screen to do a refresh
    if (on_data_changed_) {
      on_data_changed_();
    } else {
      spdlog::debug("No refresh trigger set.");
    }
  };

  auto input_field =
      Input(user_text_.get(), "Type to search and press Enter to select a file",
            input_options);

  // Component for displaying files available based on user input.
  MenuOption menu_options;
  // IMPORTANT: DO NOT pass *files as an argument since that would pass a
  // reference to the underlying vector and Menu has a constructor which when
  // receives this, creates a copy of the vector and hence any future changes in
  // the vector would not be reflected.
  menu_options.entries = available_files_.get();
  menu_options.selected = menu_index_.get();
  auto menu = Menu(menu_options);

  auto menu_container = Container::Vertical({input_field, menu});

  // input_field->TakeFocus();

  // Wrap the container in a Catch Event so as to be able to select files to be
  // added for tracking.
  auto menu_with_events = CatchEvent(menu_container, [this](Event event) {
    if (event == Event::Return) {
      if (!available_files_->empty()) {
        selected_files_->push_back(available_files_->at(*menu_index_));
        if (on_data_changed_)
          on_data_changed_();
      }
      return true;
    }
    return false;
  });

  // The local variables that the passed components like menu and input_field
  // depends on must be passed to the lambda in Renderer or those variables
  // should be defined as class members and then capture "this" inside the
  // callback.
  // IMPORTANT: Always think about how the dependencies of variables captured by
  // the lambda would be accessed. Either they should be passed as well or use
  // this if they are class members.
  return Renderer(menu_with_events, [this, menu, input_field] {
    return vbox({input_field->Render(), separator(), menu->Render() | frame}) |
           border | flex;
  });
}

Component ControlPanel::compose_selected_files_panel() {
  MenuOption menu_options{.entries = selected_files_.get(),
                          .selected = selected_menu_index_.get()};
  auto menu = Menu(menu_options);

  return Renderer(menu, [this, menu] {
    auto header = hbox(text("Selected files"));
    return vbox({header, separator(), menu->Render() | frame}) | border | flex;
  });
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
