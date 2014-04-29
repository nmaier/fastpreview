#pragma once

#include <windows.h>
#include <string>
#include <tchar.h>

#include "stringtools.h"

class Exception
{
protected:
  std::wstring error_;

public:
  Exception() : error_(_T(""))
  {}
  
  Exception(const std::wstring& aError) : error_(aError)
  {}
  
  Exception(const std::string& aError) : error_(stringtools::convert(aError))
  {}
  
  Exception(const Exception &aRef) : error_(aRef.GetString())
  {}
  
  virtual ~Exception()
  {};

  void show(HWND hwnd_, std::wstring caption = L"") const
  {
    if (caption.empty()) {
      caption = L"Error";
    }
    std::wstring msg = error_;
    if (msg.empty()) {
      msg = L"Unhandled error occured!";
    }
    MessageBox(
      hwnd_,
      msg.c_str(),
      caption.c_str(),
      MB_ICONERROR
      );
  }

  const std::wstring& GetString() const
  {
    return error_;
  }
};

class WindowsException : public Exception
{
public:
  WindowsException(void)
  {
    LPWSTR err = nullptr;
    if (FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER
      | FORMAT_MESSAGE_FROM_SYSTEM,
      nullptr,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPWSTR)&err,
      0,
      nullptr
      )) {
      error_.assign(err);
      LocalFree((LPVOID)err);
    }
    else {
      error_.assign(_T("Another unexpected Windows Error"));
    }
  }
};
