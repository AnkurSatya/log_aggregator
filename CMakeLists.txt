cmake_minimum_required(VERSION 3.28)
project(LogAggregator CXX)

find_package(ZLIB REQUIRED)

add_executable(log_aggregator
    src/main.cpp
    src/file_reader.cpp
)
target_link_libraries(log_aggregator ZLIB::ZLIB)

target_include_directories(log_aggregator PRIVATE include)
