#pragma once

class IEngineService{
 public:
  virtual void OnScanCompeleted(bool compeleted) = 0;
  virtual void OnSearch(const wchar_t* message, bool compeleted) = 0;
};

class IEngineInterface {
 public:
  virtual bool __stdcall Init(IEngineService* pService) = 0;
  virtual void __stdcall Search(const wchar_t* search_content) = 0;
};

typedef IEngineInterface*(__stdcall* pFnCreateEngine)();
#define PFN_CREATE_ENGINE_NAME "CreateEngine"