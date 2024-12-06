#pragma once

#include <Windows.h>

#include <atomic>
#include <cstdint>
#include <optional>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>

#include "EngineInterface.h"
#include "threadpool.h"
#include "FileSystemData.h"

class Engine : public IEngineInterface {
 public:
  virtual bool Init(IEngineService* service) override;
  virtual void Search(const wchar_t* search_content) override;

 private:
  void LoadFileSystemDatas();
  void OnFilesSystemDatasCompeleted();

  void OnSearchResult(const std::wstring& result, bool compelete);



 private:
  IEngineService* engine_service_ = nullptr;
  MessageLoop main_ml_;

  std::vector<FileSystemDataPtr> all_file_system_datas_;
};
