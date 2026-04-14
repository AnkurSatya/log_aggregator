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
  Viewport(const Viewport &) = delete;
  Viewport &operator=(const Viewport &) = delete;
  Viewport(Viewport &&) = delete;
  Viewport &operator=(Viewport &&) = delete;

  Viewport();
  Result<void, std::string> add_pane(FileId, std::filesystem::path);
  void remove_pane(FileId);
  void update_pane(FileId, const std::string);
  ftxui::Component compose();

private:
  std::unordered_map<FileId, FilePane> panes_;
  std::mutex pane_update_mutex_;
  ftxui::Component compose_pane(FilePane &);
};
