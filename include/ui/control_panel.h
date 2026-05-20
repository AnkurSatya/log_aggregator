#include <ftxui/component/component.hpp>
#include <set>
#include <spdlog/spdlog.h>
#include <utils/filesystem_utils.h>
#include <utils/types.h>

struct ControlPanelModel {
  // List of files(and dirs) available in the path entered by the user. It
  // changes when the user enters a different path.
  std::vector<std::string> available_files;
  // Index of the row in focus in the Menu.
  int menu_index = 0;
  std::vector<std::string> selected_files;
  std::set<std::string> unique_selected_files;
  // Filepath selected by user to be tracked. It will change when the user
  // presses the key to select a file.
  std::string selected_file;
  int selected_menu_index = 0;
  std::string user_text;

  void reset_search() {
    user_text.clear();
    available_files.clear();
    menu_index = 0;
  }

  void update_file_search() {
    if (user_text.empty()) {
      reset_search();
      return;
    }

    // ToDo: Add validations on user_text before scanning dirs.

    // Scan directory if user_text is not empty
    auto files_found = file_utils::scan_directory(user_text);
    if (files_found) {
      available_files = std::move(files_found.value());
      return;
    }

    available_files = {std::move(files_found.error().message)};
  }

  void select_current_file() {
    if (available_files.empty())
      return;

    if (menu_index >= 0 && menu_index < available_files.size()) {
      auto file_to_add = available_files.at(menu_index);
      if (unique_selected_files.count(file_to_add) != 0)
        return;
      selected_files.push_back(file_to_add);
      unique_selected_files.insert(file_to_add);
    }
  }
};

class ControlPanelView {
public:
  ControlPanelView(ControlPanelView &&other) = delete; // Moving not llowed
  ControlPanelView &
  operator=(ControlPanelView &&other) = delete; // reassignment not allowed
  ControlPanelView(const ControlPanelView &) = delete; // copying not allowed
  ControlPanelView &operator=(const ControlPanelView &) =
      delete; // copying reassignment not allowwed

  ControlPanelView(std::shared_ptr<spdlog::logger>);

  ftxui::Component get_root_container() { return root_container_; };

  void set_callback_data_change(std::function<void()> callback) {
    on_data_changed_ = std::move(callback);
  }

private:
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<ControlPanelModel> model_ =
      std::make_shared<ControlPanelModel>();
  ftxui::Component root_container_ = ftxui::Container::Vertical({});
  // Stores the screen refresh trigger.
  std::function<void()> on_data_changed_;

  void compose();
  ftxui::Component compose_selected_files_panel();
  ftxui::Component compose_searchable_menu();

  void callback_list_files();
  void callback_add_file();
  void callback_remove_file();
  void callback_save();
  void callback_cancel();
};
