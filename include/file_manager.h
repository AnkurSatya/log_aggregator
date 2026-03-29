#pragma once
#include <file_reader.h>
#include <thread>
#include <unordered_map>
#include <utils/events.h>
#include <vector>

class FileManager {
public:
  Result<FileId> add_file(const std::filesystem::path);
  void remove_file(FileId);
  void process_events(std::stop_token);
  void handle(const Events::FileError &);
  void handle(const Events::FileClosed &);
  void handle(const Events::DataAvailable &);

private:
  FileId next_file_id_{0};
  std::unordered_map<FileId, FileReader> file_readers_;
  std::unordered_map<FileId, std::jthread> threads_;
  std::vector<FileReader> files;
  std::jthread event_processor_thread_;
};
