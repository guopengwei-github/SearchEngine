#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "UsnOperator.h"
#include "threadpool.h"
#include "utils.h"

class FileSystemData {
  


 public:
  FileSystemData(const DriveInfo& drive_info);
  FileSystemData() = delete;

  ~FileSystemData();

  using PerpareCompeleteCallback = std::function<void()>;
  void AsyncPerpareData(PerpareCompeleteCallback perpare_compelete);

  using SearchResultCallback =
      std::function<void(std::wstring, bool /*compelete*/)>;
  void AsyncSearch(const std::wstring& search_content,
                   SearchResultCallback callback);

 private:
  void PerpareData(PerpareCompeleteCallback perpare_compelete);
  void Search(const std::wstring& search_content,
                   SearchResultCallback callback);

  void WatchFileChange();
  void OnWatchFileChange();

  void HandleFileEntryChange(engine_define::FileDataChangeReason reason,
                             FileAndDirectoryEntryPtr entry, bool compelete);

  void AddFileEntry(FileAndDirectoryEntryPtr entr);
  void RemoveFileEntry(FileAndDirectoryEntryPtr entr);
  void UpdateFileEntry(FileAndDirectoryEntryPtr entr);

 private:
  DriveInfo driver_info_;
  MessageLoop work_ml_;
  UsnOperator usn_operator_;

  std::vector<FileAndDirectoryEntry> all_file_entry_;

  std::atomic<bool> data_perpared_compeleted_;
  HANDLE stop_event_ = nullptr;
  std::thread watch_thread_;
};

using FileSystemDataPtr = std::unique_ptr<FileSystemData>;