cmake_minimum_required(VERSION 3.28)
project(LogAggregator CXX)

add_executable(log_aggregator
    src/main.cpp
    src/core/file_reader.cpp
    src/core/file_manager.cpp
    src/ui/ui_manager.cpp
    src/ui/control_panel.cpp
    src/ui/viewport.cpp
)

# Fetch FTXUI
include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG main
)
FetchContent_MakeAvailable(ftxui)

target_link_libraries(log_aggregator
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

target_include_directories(log_aggregator PRIVATE include)
