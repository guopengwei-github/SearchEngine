#include "pch.h"

#include "SearchEngineCLI.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

using namespace msclr::interop;

namespace SearchEngineCLI {

// Import the original DLL function
typedef IEngineInterface*(__stdcall* CreateTestFunc)(DWORD, LPDWORD);

public
ref class SearchWrapper {
 public:
  SearchWrapper() {
    HMODULE hModule = LoadLibrary(L"SearchEngine.dll");
    if (!hModule) {
      throw gcnew Exception("Failed to load DLL");
    }

    create_engine_ = (CreateTestFunc)GetProcAddress(hModule, "CreateEngine");
    if (!create_engine_) {
      throw gcnew Exception("Failed to get CreateTest function");
    }
  }

  void Init(OnSearchDelegate ^ callback, OnCompeletedDelegate ^ compeleted_callback) {
    DWORD someParameter = 0;
    search_engine_ = create_engine_(0, &someParameter);

    SearchServiceDelegate* service =
        new SearchServiceDelegate(callback, compeleted_callback);
    search_engine_->Init(service);
  }

  void Search(String ^ message) {
    std::wstring nativeWString = marshal_as<std::wstring>(message);
    search_engine_->Search(nativeWString.c_str());
  }

 private:
  CreateTestFunc create_engine_;
  IEngineInterface* search_engine_ = nullptr;
};

}  // namespace CliDll