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
    src/utils/filesystem_utils.cpp
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
SET(PROTO_IMPORT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
SET(PROTO_ROOT ${PROTO_IMPORT_ROOT}/proto)
set(PROTO_FILES
  ${PROTO_ROOT}/log_aggregator/file_events.proto
  ${PROTO_ROOT}/log_aggregator/file_commands.proto
  ${PROTO_ROOT}/log_aggregator/file_service.proto
)
protobuf_generate(
  TARGET ${EXE_NAME}
  LANGUAGE cpp
  IMPORT_DIRS  ${PROTO_IMPORT_ROOT}
  PROTOS ${PROTO_FILES}
)

# set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
# set(SPDLOG_HEADER_ONLY ON CACHE BOOL "" FORCE)
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG v2.x
)
FetchContent_MakeAvailable(spdlog)


target_link_libraries(${EXE_NAME}
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE libzmq
  PRIVATE cppzmq
  PRIVATE ${Protobuf_LIBRARIES}
  PRIVATE spdlog::spdlog
)

target_include_directories(${EXE_NAME} PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Protobuf_INCLUDE_DIRS}
  include
)
