#pragma once

#include <Windows.h>
#include <winioctl.h>

#include <functional>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "UsnEntry.h"
#include "utils.h"

struct FileAndDirectoryEntry {
 public:
  // FileAndDirectoryEntry 将使用 vector组织，以便快速查找
  // 由于 vector 删除一个节点代价非常高。因此仅标记为删除
  bool flag_deleted = false;

  UINT64 fileReferenceNumber;
  UINT64 parentFileReferenceNumber;
  std::wstring fileName;
  bool isFolder;
  std::wstring path;

  FileAndDirectoryEntry() {
    fileReferenceNumber = 0;
    parentFileReferenceNumber = 0;
    isFolder = false;
  };

  FileAndDirectoryEntry(const UsnEntry& usnEntry, const std::wstring& path) {
    fileReferenceNumber = usnEntry.FileReferenceNumber;
    parentFileReferenceNumber = usnEntry.ParentFileReferenceNumber;
    fileName = usnEntry.FileName;
    isFolder = usnEntry.IsFolder();
    this->path = path;
  }
};

using FileAndDirectoryEntryPtr = std::shared_ptr<FileAndDirectoryEntry>;

struct UsnJournalDataCustom {
  DriveInfo Drive;
  UINT64 UsnJournalID;
  INT64 FirstUsn;
  INT64 NextUsn;
  INT64 LowestValidUsn;
  INT64 MaxUsn;
  UINT64 MaximumSize;
  UINT64 AllocationDelta;

  UsnJournalDataCustom(const DriveInfo& drive,
                       const USN_JOURNAL_DATA& ntfsUsnJournalData)
      : Drive(drive),
        UsnJournalID(ntfsUsnJournalData.UsnJournalID),
        FirstUsn(ntfsUsnJournalData.FirstUsn),
        NextUsn(ntfsUsnJournalData.NextUsn),
        LowestValidUsn(ntfsUsnJournalData.LowestValidUsn),
        MaxUsn(ntfsUsnJournalData.MaxUsn),
        MaximumSize(ntfsUsnJournalData.MaximumSize),
        AllocationDelta(ntfsUsnJournalData.AllocationDelta) {}
};
using PUsnJournalDataCustom = UsnJournalDataCustom*;

class FrnFilePath {
 public:
  UINT64 fileReferenceNumber = 0;
  UINT64 parentFileReferenceNumber = 0;
  std::wstring fileName;
  std::wstring path;

 public:
  FrnFilePath() = default;

  // Constructor
  FrnFilePath(UINT64 fileRef, UINT64 parentRef, const std::wstring& file,
              const std::wstring& p = L"")
      : fileReferenceNumber(fileRef),
        parentFileReferenceNumber(parentRef),
        fileName(file),
        path(p) {}

  UINT64 getFileReferenceNumber() const { return fileReferenceNumber; }

  UINT64 getParentFileReferenceNumber() const {
    return parentFileReferenceNumber;
  }

  // Getter for FileName
  const std::wstring& getFileName() const { return fileName; }

  // Getter and Setter for Path
  const std::wstring& getPath() const { return path; }

  void setPath(const std::wstring& newPath) { path = newPath; }
};

class UsnOperator {
 public:
  UsnOperator(DriveInfo drive);

  std::vector<FileAndDirectoryEntry> LoadAllFilesAndDirectories();

  using ChangeRecordRepatCallback =
      std::function<void(engine_define::FileDataChangeReason,
                         FileAndDirectoryEntryPtr, bool /*compelete*/)>;
  // 用于查询变化数据
  void QueryLatestChangeData(ChangeRecordRepatCallback change_repated_record);

  std::vector<UsnEntry> GetEntries();

 private:
  DWORD QueryLatestUSNJournal(USN_JOURNAL_DATA& latest_usn_journal_data);

  void QueryChangeDataSinceUsn(USN& usnid,
                               ChangeRecordRepatCallback change_repated_record);

  void AsyncWatchUSNChange();
  void WatchUSNChange();
  HANDLE GetRootHandle();

  void USNRecordChange(const USN_RECORD* usn_record);

  std::wstring GetPathByParentFrn(DWORDLONG parent_frn);

  std::vector<FrnFilePath> GetFolderPath(const std::vector<UsnEntry>& folders,
                                         const std::wstring& driveName);
  static std::wstring BuildPath(FrnFilePath currentNode,
                                FrnFilePath parentNode);

 private:
  USN_JOURNAL_DATA last_ntfs_usn_journal_data_;
  DriveInfo drive_info_;
  HANDLE drive_handle_;
  std::thread monitor_thread_;

  struct EntryChangeInfo {
    FileAndDirectoryEntryPtr entry;
    engine_define::FileDataChangeReason reason;
  };

  std::unordered_map<USN, EntryChangeInfo> changed_file_entry_;

  std::unordered_map<uint64_t, std::wstring> frn_path_map_;

  bool is_scan_compeleted_ = false;
};
