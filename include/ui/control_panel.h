#include <filesystem>
#include <ftxui/component/component.hpp>
#include <spdlog/spdlog.h>
#include <utils/types.h>

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
  ftxui::Component root_container_ = ftxui::Container::Vertical({});

  // Stores the screen refresh trigger.
  std::function<void()> on_data_changed_;

  void compose();
  ftxui::Component compose_file_browser();
  void compose_selected_files_panel();
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
