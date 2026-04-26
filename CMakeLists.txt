cmake_minimum_required(VERSION 3.28)
project(LogAggregator CXX)
set(EXE_NAME log_aggregator)

add_executable(${EXE_NAME}
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

FetchContent_Declare(libzmq
  GIT_REPOSITORY https://github.com/zeromq/libzmq
  GIT_TAG master
)
# Disable tests and extras to speed up your build
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_PERF_TOOL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libzmq)


FetchContent_Declare(cppzmq
  GIT_REPOSITORY https://github.com/zeromq/cppzmq
  GIT_TAG master
)
set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cppzmq)

find_package(Protobuf REQUIRED)
set(PROTO_FILES
  proto/log_aggregator/file_events.proto
  proto/log_aggregator/file_commands.proto
)
protobuf_generate(
  TARGET ${EXE_NAME}
  LANGUAGE cpp
  IMPORT_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/proto
  PROTOS ${PROTO_FILES}
)


target_link_libraries(${EXE_NAME}
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE libzmq
  PRIVATE cppzmq
  PRIVATE ${Protobuf_LIBRARIES}
)

target_include_directories(${EXE_NAME} PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Protobuf_INCLUDE_DIRS}
  include
)
