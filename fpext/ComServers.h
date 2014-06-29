#pragma once

#include <windows.h>
#include <string>

namespace COMServers {
  using std::wstring;

  HMODULE GetModuleHandle();

  bool Register(
    const wstring& name, const wstring& description, const unsigned char version,
    const wstring& CLSID
    );

  bool Unregister(
    const wstring& name, const unsigned char version, const wstring& CLSID);

  const wstring ModulePath();
  const wstring ModuleDirectory();
};
