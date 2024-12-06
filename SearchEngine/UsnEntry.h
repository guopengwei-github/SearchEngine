#pragma once

#include <Windows.h>
#include <winioctl.h>

#include <string>


struct UsnEntry {
 public:
  UINT32 RecordLength;
  UINT64 FileReferenceNumber;
  UINT64 ParentFileReferenceNumber;
  INT64 Usn;
  UINT32 Reason;
  UINT32 FileAttributes;
  INT32 FileNameLength;
  INT32 FileNameOffset;
  std::wstring FileName;

  bool IsFolder() const { return (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }

 public:
  UsnEntry(const USN_RECORD_V2& usnRecord) {
    RecordLength = usnRecord.RecordLength;
    FileReferenceNumber = usnRecord.FileReferenceNumber;
    ParentFileReferenceNumber = usnRecord.ParentFileReferenceNumber;
    Usn = usnRecord.Usn;
    Reason = usnRecord.Reason;
    FileAttributes = usnRecord.FileAttributes;
    FileNameLength = usnRecord.FileNameLength;
    FileNameOffset = usnRecord.FileNameOffset;
    FileName = std::wstring(usnRecord.FileName,
                            usnRecord.FileNameLength / sizeof(WCHAR));
  }
};

