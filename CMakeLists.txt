cmake_minimum_required(VERSION 3.28)
project(log_aggregator CXX)

find_package(ZLIB REQUIRED)

add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
