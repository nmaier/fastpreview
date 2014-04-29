#include "stringtools.h"
#include <windows.h>
#include <memory>
#include <strsafe.h>

using namespace std;

#if defined(FORCE_DUMP) || defined(_DEBUG)
wchar_t dump_buf[10240];
std::wstring dump_prefix(
  L"FastPreviewDbg"
#ifdef _WIN64
  L"(x86_64)"
#else
  L"(x86): "
#endif
  );

void  __cdecl dump(const wchar_t* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  StringCchVPrintf(dump_buf, 10240, fmt, ap);
  OutputDebugString((dump_prefix + dump_buf + L"\n").c_str());
  va_end(ap);
}
#endif

string stringtools::WStringToString(const wchar_t *aIn)
{
  const int x = WideCharToMultiByte(CP_ACP, 0, aIn, -1, NULL, 0, NULL, NULL);
  if (x == 0) {
    return "";
  }

  auto buf = make_unique <char[]>(x + 1);
  string rv;
  if (WideCharToMultiByte(CP_ACP, 0, aIn, -1, buf.get(), x + 1, NULL, NULL)) {
    rv = buf.get();
  }
  return rv;
}

wstring stringtools::StringToWString(const char *aIn, unsigned int codePage)
{
  const int x = MultiByteToWideChar(codePage, 0, aIn, -1, NULL, 0);
  auto buf = make_unique<wchar_t[]>(x + 1);
  wstring rv;
  if (MultiByteToWideChar(codePage, 0, aIn, -1, buf.get(), x + 1)) {
    rv = buf.get();
  }
  return rv;
}