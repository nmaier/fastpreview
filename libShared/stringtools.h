#pragma once

#include <windows.h>
#include <memory>
#include <string>

namespace stringtools {

  std::string WStringToString(const wchar_t *aIn);

  inline std::string WStringToString(const std::wstring& aIn)
  {
    return WStringToString(aIn.c_str());
  }

  std::wstring StringToWString(const char *aIn, unsigned int codePage = 0);

  inline std::wstring StringToWString(const std::string& aIn, unsigned int codePage = 0)
  {
    return StringToWString(aIn.c_str(), codePage);
  }

  inline std::wstring itos(int aInt)
  {
    wchar_t num[32];
    _itow_s(aInt, num, 32, 10);
    return std::wstring(num);
  }

  inline std::wstring convert(const char *b, unsigned int codePage = 0)
  {
    return StringToWString(b, codePage);
  }

  inline std::wstring convert(const std::string& b, unsigned int codePage = 0)
  {
    return StringToWString(b, codePage);
  }

  inline std::wstring convert(int b)
  {
    return itos(b);
  }

  inline HINSTANCE getImageModuleHandle()
  {
    HINSTANCE rv = 0;
    return GetModuleHandleEx(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      (LPCTSTR)getImageModuleHandle,
      &rv
      ) ? rv : 0;
  }

  inline std::wstring loadResourceString(UINT id)
  {
    std::wstring rv;
    wchar_t buf[0xff];
    if (::LoadString(getImageModuleHandle(), id, buf, 0xff)) {
      rv.assign(buf);
    }
    return rv;
  }

  inline std::wstring __cdecl formatResourceString(unsigned int resource, ...)
  {
    std::wstring rv = loadResourceString(resource);
    if (!rv.empty()) {
      va_list args;
      va_start(args, resource);
      wchar_t *msg = 0;
      DWORD res = ::FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        rv.c_str(),
        0,
        0,
        reinterpret_cast<LPWSTR>(&msg),
        0xffff,
        &args
        );
      va_end(args);
      if (res) {
        rv = msg;
        LocalFree(msg);
      }
    }
    return rv;
  }
};

template<typename T>
inline std::wstring itos(const T t)
{
  std::wstringstream ss;
  ss << t;
  return ss.str();
}

#if defined(FORCE_DUMP) || defined(_DEBUG)
extern wchar_t dump_buf[10240];
extern std::wstring dump_prefix;

void __cdecl dump(const wchar_t* fmt, ...);
#else
#define dump(...)
#endif