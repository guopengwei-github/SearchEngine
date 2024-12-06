#pragma once

#include <Windows.h>

#include <string>
#include <vector>
#include <algorithm>

namespace engine_define  {
enum  FileDataChangeReason  { Unknown, Create, Delete, Rename };
}

struct DriveInfo {
  std::wstring volume_name; // Èç C:\¡£
  std::wstring drive_letter; // Èç c
  std::wstring file_system_name;  // ntfs,fat
  DWORD type = 0;
};

namespace utils {
std::vector<DriveInfo> ListDrives();

bool contains(const std::string& str1, const std::string& str2);

std::wstring toLower(const std::wstring& str);

bool containsIgnoreCase(const std::wstring& str1, const std::wstring& str2);

};  // namespace utils
