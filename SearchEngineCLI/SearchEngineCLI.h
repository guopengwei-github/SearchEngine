#pragma once


#include <Windows.h>

#include <vcclr.h>  // For gcroot
#include "../SearchEngine/EngineInterface.h"

using namespace System;

namespace SearchEngineCLI {
public
delegate void OnSearchDelegate(String ^ message, bool compeleted);
public delegate void OnCompeletedDelegate(bool compeleted);
	
 class SearchServiceDelegate : public IEngineService
	{
 public:
   SearchServiceDelegate(OnSearchDelegate ^ search_callback,
                        OnCompeletedDelegate ^ compeleted_callback)
      : search_callback_(search_callback),
        compelete_callback_(compeleted_callback) {
   
   }

  void OnSearch(const wchar_t* message, bool compeleted) override {
    String ^ managedMessage = gcnew String(message);
    search_callback_->Invoke(managedMessage, compeleted);
  }

  virtual void OnScanCompeleted(bool compeleted) override {
    compelete_callback_->Invoke(compeleted);
  }


 private:
  gcroot<OnSearchDelegate ^> search_callback_;
  gcroot<OnCompeletedDelegate ^> compelete_callback_;
	};
}
