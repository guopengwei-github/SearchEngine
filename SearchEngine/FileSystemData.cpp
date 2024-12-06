#include "FileSystemData.h"

#include "utils.h"

FileSystemData::FileSystemData(const DriveInfo& drive_info)
    : driver_info_(drive_info), usn_operator_(drive_info) {
  stop_event_ = CreateEvent(NULL, TRUE, FALSE, NULL);
}

FileSystemData::~FileSystemData() {
  if (stop_event_) {
    SetEvent(stop_event_);
    CloseHandle(stop_event_);
  }
  work_ml_.Stop();
}

void FileSystemData::AsyncPerpareData(PerpareCompeleteCallback callback) {
  work_ml_.EnqueueNoResult(
      std::bind(&FileSystemData::PerpareData, this, std::move(callback)));
}

void FileSystemData::AsyncSearch(const std::wstring& search_content,
                                 SearchResultCallback callback) {
  if (!data_perpared_compeleted_.load()) {
    return;
  }

  work_ml_.EnqueueNoResult(std::bind(&FileSystemData::Search, this,
                                     search_content, std::move(callback)));
}

void FileSystemData::PerpareData(PerpareCompeleteCallback callback) {
  all_file_entry_ = usn_operator_.LoadAllFilesAndDirectories();

  data_perpared_compeleted_.store(true);
  callback();

  watch_thread_ = std::thread(&FileSystemData::WatchFileChange, this);
}

void FileSystemData::Search(const std::wstring& search_content,
                            SearchResultCallback callback) {
#pragma omp parallel for
  for (int i = 0; i < all_file_entry_.size(); ++i) {
    const auto& file = all_file_entry_[i];
    if (file.flag_deleted) {
      continue;
    }

    const auto& fileName = file.fileName;

    if (utils::containsIgnoreCase(fileName, search_content)) {
      std::wstring full_path = file.path + L"\\" + file.fileName;
      callback(full_path, false);
    }
  }

  callback(L"", true);
}

void FileSystemData::WatchFileChange() {
  std::wstring wstr_volum_path;
  wstr_volum_path = driver_info_.drive_letter + L":\\";

  HANDLE h_monitor = FindFirstChangeNotification(
      wstr_volum_path.c_str(), TRUE,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
  if (INVALID_HANDLE_VALUE != h_monitor) {
    HANDLE handles[] = {stop_event_, h_monitor};
    constexpr DWORD kTimeout = 20000;
    while (true) {
      DWORD ret = WaitForMultipleObjects(2, handles, false, kTimeout);
      if (ret == WAIT_OBJECT_0) {
        break;
      } else if (ret == WAIT_OBJECT_0 + 1) {
        FindNextChangeNotification(h_monitor);
        OnWatchFileChange();
      } else if (ret == WAIT_TIMEOUT) {
        // 超时，主动读取一次？
        // OnWatchFileChange();
      }
    }

    FindCloseChangeNotification(h_monitor);
    h_monitor = INVALID_HANDLE_VALUE;
  }
}

void FileSystemData::OnWatchFileChange() {
  usn_operator_.QueryLatestChangeData(
      [this](engine_define::FileDataChangeReason reason,
             FileAndDirectoryEntryPtr entry, bool compelete) {
        work_ml_.EnqueueNoResult(
            std::bind(&FileSystemData::HandleFileEntryChange, this, reason,
                      std::move(entry), compelete));
      });
}

void FileSystemData::HandleFileEntryChange(
    engine_define::FileDataChangeReason reason, FileAndDirectoryEntryPtr entry,
    bool compelete) {
  switch (reason) {
    case engine_define::Unknown:
      break;
    case engine_define::Create:
      AddFileEntry(std::move(entry));
      break;
    case engine_define::Delete:
      RemoveFileEntry(std::move(entry));
      break;
    case engine_define::Rename:
      UpdateFileEntry(std::move(entry));
      break;
    default:
      break;
  }
}

void FileSystemData::AddFileEntry(FileAndDirectoryEntryPtr entr) {
  all_file_entry_.push_back(std::move(*entr));
}

void FileSystemData::RemoveFileEntry(FileAndDirectoryEntryPtr entr) {
  for (auto &entry : all_file_entry_) {
    if (entry.fileReferenceNumber == entr->fileReferenceNumber) {
      entry.flag_deleted = true;
      break;
    }
  }
}

void FileSystemData::UpdateFileEntry(FileAndDirectoryEntryPtr entr) {
  for (int i = 0; i < all_file_entry_.size(); ++i) {
    auto& file = all_file_entry_[i];
    if (file.fileReferenceNumber == entr->fileReferenceNumber) {
      file = *entr;
      break;
    }
  }
}
