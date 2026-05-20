#include <filesystem>
#include <ftxui/component/component.hpp>
#include <set>
#include <spdlog/spdlog.h>
#include <utils/types.h>

struct ControlPanelModel {
  std::vector<std::string> available_files;
  // Index of the row in focus in the Menu.
  int menu_index = 0;
  std::set<std::string> selected_files;
  // Filepath selected by user to be tracked. It will change when the user
  // presses the key to select a file.
  // std::shared_ptr<std::string> selected_file_;
  int selected_menu_index = 0;
  std::string user_text;
};

class ControlPanel {
public:
  ControlPanel(ControlPanel &&other) = delete; // Moving not llowed
  ControlPanel &
  operator=(ControlPanel &&other) = delete;    // reassignment not allowed
  ControlPanel(const ControlPanel &) = delete; // copying not allowed
  ControlPanel &
  operator=(const ControlPanel &) = delete; // copying reassignment not allowwed

  ControlPanel(std::shared_ptr<spdlog::logger>);

  ftxui::Component get_root_container() { return root_container_; };

  void set_callback_data_change(std::function<void()> callback) {
    on_data_changed_ = std::move(callback);
  }

private:
  std::shared_ptr<spdlog::logger> logger_;
  // List of files(and dirs) available in the path entered by the user. It
  // changes when the user enters a different path.
  std::shared_ptr<std::vector<std::string>> available_files_ =
      std::make_shared<std::vector<std::string>>();
  // Index of the row in focus in the Menu.
  std::shared_ptr<int> menu_index_ = std::make_shared<int>(0);
  std::shared_ptr<std::vector<std::string>> selected_files_ =
      std ::make_shared<std::vector<std ::string>>();
  // Filepath selected by user to be tracked. It will change when the user
  // presses the key to select a file.
  std::shared_ptr<std::string> selected_file_;
  std::shared_ptr<int> selected_menu_index_ = std::make_shared<int>(0);
  std::shared_ptr<std::string> user_text_ = std::make_shared<std::string>();

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

  Result<std::vector<std::string>, log_aggregator::Error>
      scan_directory(std::string_view);

  bool is_valid_directory(std::filesystem::path);
};
