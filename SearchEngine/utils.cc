#include "utils.h"

#include <Shlwapi.h>

#include <vector>

#pragma comment(lib, "Shlwapi.lib")

namespace utils {

std::vector<DriveInfo> ListDrives() {
  std::vector<DriveInfo> drivers;

  for (uint16_t i = 0x63; i < 0x7b; ++i) {
    wchar_t file_system_name[MAX_PATH] = {0};

    wchar_t disk_volume[3] = {0};
    disk_volume[0] = i;
    disk_volume[1] = L':';
    if (PathFileExistsW(disk_volume) &&
        DRIVE_FIXED == GetDriveType(disk_volume)) {
      const char volume = static_cast<char>(i);

      BOOL success = GetVolumeInformationW(
          disk_volume,       // ������·��
          nullptr,           // ��������꣨����Ҫ��
          0,                 // ��껺������С
          nullptr,           // ���кţ�����Ҫ��
          nullptr,           // ���������ȣ�����Ҫ��
          nullptr,           // �ļ�ϵͳ��־������Ҫ��
          file_system_name,  // ���ڽ����ļ�ϵͳ���ƵĻ�����
          _countof(file_system_name)  // ��������С���ַ�����
      );

      if (success) {
        DriveInfo driver;
        driver.volume_name = disk_volume;
        driver.drive_letter = volume;
        driver.file_system_name = file_system_name;
        driver.type = GetDriveType(disk_volume);
        drivers.push_back(std::move(driver));
      }
    }
  }

  return drivers;
}

bool contains(const std::string& str1, const std::string& str2) {
  return str1.find(str2) != std::string::npos;
}

std::wstring toLower(const std::wstring& str) {
  std::wstring lowerStr = str;
  std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
  return lowerStr;
}

bool containsIgnoreCase(const std::wstring& str1, const std::wstring& str2) {
  std::wstring lowerStr1 = toLower(str1);
  std::wstring lowerStr2 = toLower(str2);
  return lowerStr1.find(lowerStr2) != std::string::npos;
}

};  // namespace utils
