#include <filesystem>
#include <ftxui/component/component.hpp>
#include <utils/types.h>

struct FilePane {
  FileId file_id;
  std::filesystem::path path;
  std::deque<std::string> data;
};

class Viewport {
public:
  // This would be set by UIManager since this callback needs to communicate
  // with core::FileManager.
  Viewport(const Viewport &) = delete;
  Viewport &operator=(const Viewport &) = delete;
  Viewport(Viewport &&) = delete;
  Viewport &operator=(Viewport &&) = delete;

  Viewport();
  void set_callback_pane_close(std::function<void(FileId)>);
  Result<void, std::string> add_pane(FileId, std::filesystem::path);
  void remove_pane(FileId);
  void update_pane(FileId, const std::string);
  ftxui::Component compose();
  ftxui::Element buildPaneGrid();
  ftxui::Component get_root_container() { return root_container_; };

private:
  std::unordered_map<FileId, FilePane> panes_;
  std::unordered_map<FileId, ftxui::Component> views_;
  std::mutex pane_update_mutex_;
  std::function<void(FileId)> callback_pane_close_;
  ftxui::Component root_container_ = ftxui::Container::Vertical({});
  ftxui::Component compose_pane(FileId);
  const FilePane *get_pane(FileId);
};
