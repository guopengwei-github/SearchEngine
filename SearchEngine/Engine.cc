
#include "Engine.h"

#include "utils.h"

bool Engine::Init(IEngineService* service) {
  engine_service_ = service;

  LoadFileSystemDatas();


  return true;
}

void Engine::Search(const wchar_t* search_content) {
  for (auto& file_system_data : all_file_system_datas_) {
    file_system_data->AsyncSearch(
        search_content, [this](const std::wstring result, bool is_compelete) {
          main_ml_.EnqueueNoResult(
              std::bind(&Engine::OnSearchResult, this, result, is_compelete));
        });
  }
}

void Engine::LoadFileSystemDatas() {
  static std::atomic<int> perparing_file_system_count = 0;

  for (const auto driver : utils::ListDrives()) {
    if (driver.file_system_name != L"NTFS" || driver.drive_letter != L"e") {
      continue;
    }

    perparing_file_system_count.fetch_add(1);

    FileSystemDataPtr file_system_data_ptr =
        std::make_unique<FileSystemData>(driver);
    file_system_data_ptr->AsyncPerpareData(
        [this]() {
          perparing_file_system_count.fetch_sub(1);
          if (perparing_file_system_count.load() == 0) {
            main_ml_.EnqueueNoResult(
                std::bind(&Engine::OnFilesSystemDatasCompeleted, this));
          }
        });

    all_file_system_datas_.push_back(std::move(file_system_data_ptr));
  }
}

void Engine::OnFilesSystemDatasCompeleted() {
  engine_service_->OnScanCompeleted(true);
}

void Engine::OnSearchResult(const std::wstring& result, bool compelete) {
  engine_service_->OnSearch(result.c_str(), compelete);
}
