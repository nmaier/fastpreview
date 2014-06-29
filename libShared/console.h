#pragma once

#ifdef _DEBUG

#include <windows.h>
#include <string>
#include <tchar.h>

using namespace std;

class Console
{
  HANDLE console_;
  CRITICAL_SECTION s_;
public:
  Console()
  {
    AllocConsole();
    console_ = GetStdHandle(STD_OUTPUT_HANDLE);
    InitializeCriticalSection(&s_);
  }
  ~Console()
  {
    DeleteCriticalSection(&s_);
    FreeConsole();
  }
  void write(wstring str)
  {
    str.append(L"\n");
    EnterCriticalSection(&s_);
    DWORD cc;
    WriteConsole(
      console_,
      str.c_str(),
      static_cast<DWORD>(str.length()),
      &cc,
      NULL
      );
    LeaveCriticalSection(&s_);
  }
};

extern Console console;
#define ConWrite(x) console.write(x)

#else // _DEBUG
#define ConWrite(x)
#endif // _DEBUG