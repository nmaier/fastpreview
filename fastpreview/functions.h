#pragma once

#include <windows.h>
#include <shlobj.h>
#include "stringtools.h"

template<typename T>
struct com_releaser
{
  inline void operator()(T* t)
  {
    if (t) t->Release();
  }
};

template<typename T>
using ComPtr = std::unique_ptr<T, com_releaser<T>>;

void ExplorerMenu(const HWND& hwnd, const std::wstring &aIn);