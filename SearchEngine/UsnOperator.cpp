#include "UsnOperator.h"

#include <iostream>
#include <memory>
#include <string>

UsnOperator::UsnOperator(DriveInfo drive) {
  drive_info_ = drive;
  drive_handle_ = GetRootHandle();
}

DWORD UsnOperator::QueryLatestUSNJournal(
    USN_JOURNAL_DATA& latest_usn_journal_data) {
  DWORD size_usn_journal_data = sizeof(USN_JOURNAL_DATA);
  USN_JOURNAL_DATA temp_usn_journal_data;
  DWORD bytes_returned_count = 0;

  BOOL is_success =
      DeviceIoControl(drive_handle_, FSCTL_QUERY_USN_JOURNAL,
                      nullptr,  // No input data
                      0, &temp_usn_journal_data, size_usn_journal_data,
                      &bytes_returned_count, nullptr);

  if (is_success) {
    latest_usn_journal_data = temp_usn_journal_data;
    return NO_ERROR;
  } else {
    return GetLastError();
  }
}

void UsnOperator::QueryLatestChangeData(
    ChangeRecordRepatCallback change_repated_record) {
  USN_JOURNAL_DATA temp_journal_data;
  if (NO_ERROR != QueryLatestUSNJournal(temp_journal_data)) {
    return;
  }

  // 上一次最新的usn记录，应该处于这一次查询的范围内
  USN last_usn = last_ntfs_usn_journal_data_.NextUsn;

  if (last_usn < temp_journal_data.FirstUsn ||
      last_usn > temp_journal_data.NextUsn) {
    // range error todo:
    return;
  }

  last_ntfs_usn_journal_data_ = temp_journal_data;

  constexpr size_t kReadBufSize = 1024 * 1024;
  BYTE* buffer = new (std::nothrow) BYTE[kReadBufSize];
  if (!buffer) {
    // todo: new buffer failed.
    return;
  }

  READ_USN_JOURNAL_DATA_V0 rujd = {
      last_usn, 0xffffffff /*to get all reason*/,        0, 0,
      0,        last_ntfs_usn_journal_data_.UsnJournalID};

  DWORD bytes_returned = 0;

  changed_file_entry_.clear();

  while (true) {
    BOOL ok = DeviceIoControl(drive_handle_, FSCTL_READ_USN_JOURNAL, &rujd,
                              sizeof(rujd), buffer, kReadBufSize,
                              &bytes_returned, NULL);
    if (!ok || (bytes_returned <= sizeof(USN))) {
      break;
    }

    rujd.StartUsn = *(USN*)buffer;  // 作用，跳出该循环？
    USN_RECORD* pUsnRecord = (PUSN_RECORD)&buffer[sizeof(USN)];
    while ((PBYTE)pUsnRecord < (buffer + bytes_returned)) {
      // 记录日志变化
      USNRecordChange(pUsnRecord);
      pUsnRecord = (PUSN_RECORD)((PBYTE)pUsnRecord + pUsnRecord->RecordLength);
    }
  }

  for (auto& changed_entry : changed_file_entry_) {
    change_repated_record(changed_entry.second.reason,
                          std::move(changed_entry.second.entry), false);
  }

  changed_file_entry_.clear();
}

std::vector<FileAndDirectoryEntry> UsnOperator::LoadAllFilesAndDirectories() {
  std::vector<FileAndDirectoryEntry> files_vec;

  std::vector<UsnEntry> usn_entries = GetEntries();
  std::vector<UsnEntry> folder;

  for (const auto& usn : usn_entries) {
    if (usn.IsFolder()) {
      folder.push_back(usn);
    }
  }

  std::vector<FrnFilePath> folder_path =
      GetFolderPath(folder, drive_info_.volume_name);

  for (const auto& folder_item : folder_path) {
    frn_path_map_[folder_item.fileReferenceNumber] = folder_item.path;
  }

  for (const auto& usn : usn_entries) {
    std::wstring path = GetPathByParentFrn(usn.ParentFileReferenceNumber);
    if (!path.empty()) {
      FileAndDirectoryEntry fd_entry(usn, std::move(path));
      files_vec.emplace_back(std::move(fd_entry));
    }

    // auto it = frn_path_map_.find(usn.ParentFileReferenceNumber);
    // if (it != frn_path_map_.end()) {
    //   FileAndDirectoryEntry fd_entry(usn, it->second);
    //   files_vec.emplace_back(std::move(fd_entry));
    // }
  }

  return files_vec;
}

std::vector<UsnEntry> UsnOperator::GetEntries() {
  std::vector<UsnEntry> result;

  DWORD usn_error_code = QueryLatestUSNJournal(last_ntfs_usn_journal_data_);
  if (usn_error_code == 0) {
    MFT_ENUM_DATA_V0 mft_enum_data = {};
    mft_enum_data.StartFileReferenceNumber = 0;
    mft_enum_data.LowUsn = 0;
    mft_enum_data.HighUsn =
        last_ntfs_usn_journal_data_.NextUsn;  // 使用最新 USN;

    DWORD size_mft_enum_data = sizeof(mft_enum_data);
    DWORD kOutBufferDataSize = 1000;

    BYTE* byte_mft_enum_data(new BYTE[size_mft_enum_data]);
    memset(byte_mft_enum_data, 0, size_mft_enum_data);
    BYTE* out_byte_data(new BYTE[kOutBufferDataSize]);
    memset(out_byte_data, 0, kOutBufferDataSize);

    memcpy(byte_mft_enum_data, &mft_enum_data, size_mft_enum_data);

    DWORD outBytesCount = 0;

    while (DeviceIoControl(drive_handle_, FSCTL_ENUM_USN_DATA,
                           byte_mft_enum_data, size_mft_enum_data,
                           out_byte_data, kOutBufferDataSize, &outBytesCount,
                           nullptr)) {
      if (outBytesCount < sizeof(USN)) {
        break;  // 数据不足时退出
      }

      // 指针跳过 USN 文件引用号，直接指向 USN 记录数据
      BYTE* ptrUsnRecord = out_byte_data + sizeof(USN);

      while (outBytesCount > 60) {  // 确保有足够的字节解析一个完整记录
        USN_RECORD* usnRecord = reinterpret_cast<USN_RECORD*>(ptrUsnRecord);
        UsnEntry ue(*usnRecord);

        result.push_back(ue);

        ptrUsnRecord += usnRecord->RecordLength;
        outBytesCount -= usnRecord->RecordLength;
      }

      memcpy(byte_mft_enum_data, out_byte_data, sizeof(USN));
    }
  }

  return result;
}

void UsnOperator::AsyncWatchUSNChange() {
  monitor_thread_ = std::thread(&UsnOperator::WatchUSNChange, this);
}

void UsnOperator::WatchUSNChange() {
  std::wstring wstr_volum_path;
  wstr_volum_path = drive_info_.volume_name + L"\\";
  HANDLE h_monitor = FindFirstChangeNotification(
      wstr_volum_path.c_str(), TRUE,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
  if (INVALID_HANDLE_VALUE != h_monitor) {
    HANDLE handles[] = {h_monitor};
    constexpr DWORD kTimeout = 20000;
    while (true) {
      DWORD ret = WaitForMultipleObjects(2, handles, false, kTimeout);
      if (ret == WAIT_OBJECT_0) {
        break;
      } else if (ret == WAIT_OBJECT_0) {
        FindNextChangeNotification(h_monitor);
        // ReadUsnJournal();
      } else if (ret == WAIT_TIMEOUT) {
      }
    }

    FindCloseChangeNotification(h_monitor);
    h_monitor = INVALID_HANDLE_VALUE;
  }

  return;
}

std::vector<FrnFilePath> UsnOperator::GetFolderPath(
    const std::vector<UsnEntry>& folders, const std::wstring& driveName) {
  std::unordered_map<UINT64, FrnFilePath> pathDic = {};

  // 添加根目录路径
  constexpr UINT64 ROOT_FILE_REFERENCE_NUMBER = 0x5000000000005L;
  pathDic[ROOT_FILE_REFERENCE_NUMBER] =
      FrnFilePath(ROOT_FILE_REFERENCE_NUMBER, 0, L"", driveName);

  // 填充 pathDic
  for (const auto& folder : folders) {
    pathDic[folder.FileReferenceNumber] =
        FrnFilePath(folder.FileReferenceNumber,
                    folder.ParentFileReferenceNumber, folder.FileName);
  }

  std::stack<UINT64> treeWalkStack;

  for (const auto& [key, value] : pathDic) {
    treeWalkStack = std::stack<UINT64>();  // 清空栈
    FrnFilePath& currentValue = pathDic[key];

    if (currentValue.path.empty() && currentValue.parentFileReferenceNumber &&
        pathDic.count(currentValue.parentFileReferenceNumber) > 0) {
      FrnFilePath* parentValue =
          &pathDic[currentValue.parentFileReferenceNumber];

      while (parentValue->path.empty() &&
             parentValue->parentFileReferenceNumber &&
             pathDic.count(parentValue->parentFileReferenceNumber) > 0) {
        FrnFilePath* temp = &currentValue;
        currentValue = *parentValue;

        if (currentValue.parentFileReferenceNumber &&
            pathDic.count(currentValue.parentFileReferenceNumber) > 0) {
          treeWalkStack.push(temp->fileReferenceNumber);
          parentValue = &pathDic[currentValue.parentFileReferenceNumber];
        } else {
          parentValue = nullptr;
          break;
        }
      }

      if (parentValue != nullptr) {
        currentValue.path = BuildPath(currentValue, *parentValue);

        while (!treeWalkStack.empty()) {
          UINT64 walkedKey = treeWalkStack.top();
          treeWalkStack.pop();

          FrnFilePath& walkedNode = pathDic[walkedKey];
          FrnFilePath& parentNode =
              pathDic[walkedNode.parentFileReferenceNumber];

          walkedNode.path = BuildPath(walkedNode, parentNode);
        }
      }
    }
  }

  // 筛选路径结果
  std::vector<FrnFilePath> result;
  for (const auto& [_, path] : pathDic) {
    if (!path.path.empty() && path.path.find(driveName) == 0) {
      result.push_back(path);
    }
  }

  return result;
}

std::wstring UsnOperator::BuildPath(FrnFilePath currentNode,
                                    FrnFilePath parentNode) {
  return parentNode.path + L"\\" + currentNode.fileName;
}

HANDLE UsnOperator::GetRootHandle() {
  std::wstring volume = L"\\\\.\\" + drive_info_.volume_name;
  HANDLE volume_handle = CreateFileW(volume.c_str(), GENERIC_READ,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     nullptr, OPEN_EXISTING, 0, nullptr);

  return volume_handle;
}

void UsnOperator::USNRecordChange(const USN_RECORD* usn_record) {
  const bool del = (usn_record->Reason & USN_REASON_FILE_DELETE);
  const bool create = (usn_record->Reason & USN_REASON_FILE_CREATE);
  const bool old_name = (usn_record->Reason & USN_REASON_RENAME_OLD_NAME);
  const bool new_name = (usn_record->Reason & USN_REASON_RENAME_NEW_NAME);

  UsnEntry ue(*usn_record);
  std::wstring path = GetPathByParentFrn(usn_record->ParentFileReferenceNumber);

  std::wstring file_name = usn_record->FileName;

  std::wstring result = L" file_reference_number: " +
                        std::to_wstring(usn_record->FileReferenceNumber) +
                        L", file_name: " + file_name + L", REASON:" +
                        std::to_wstring(usn_record->Reason) + L"\n";

  OutputDebugString(result.c_str());

  FileAndDirectoryEntryPtr entry =
      std::make_shared<FileAndDirectoryEntry>(ue, path);

  engine_define::FileDataChangeReason reason = engine_define::Unknown;

  if (del)
    reason = engine_define::Delete;
  else if (create)
    reason = engine_define::Create;
  else if (new_name)
    reason = engine_define::Rename;

  if (reason != engine_define::Unknown) {
    EntryChangeInfo change_info = {std::move(entry), reason};
    changed_file_entry_[change_info.entry->fileReferenceNumber] = change_info;
  }
}

std::wstring UsnOperator::GetPathByParentFrn(DWORDLONG parent_frn) {
  auto it = frn_path_map_.find(parent_frn);
  if (it != frn_path_map_.end()) {
    return it->second;
  }

  return std::wstring();
}

void UsnOperator::QueryChangeDataSinceUsn(
    USN& usnid, ChangeRecordRepatCallback change_repated_record) {}
