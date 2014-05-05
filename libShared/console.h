#pragma once

#ifdef _DEBUG
#include <windows.h>
#include <string>
#include <tchar.h>
using namespace std;
class Console
{
  HANDLE hConsole;
  CRITICAL_SECTION s;
public:
  Console()
  {
    AllocConsole();
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    InitializeCriticalSection(&s);
  }
  ~Console()
  {
    DeleteCriticalSection(&s);
    FreeConsole();
  }
  void write(wstring str)
  {
    str.append(L"\n");
    EnterCriticalSection(&s);
    DWORD cc;
    WriteConsole(
      hConsole,
      str.c_str(),
      static_cast<DWORD>(str.length()),
      &cc,
      NULL
      );
    LeaveCriticalSection(&s);
  }
}
extern console;
#define ConWrite(x) console.write(x)
#else
#define ConWrite(x)
#endif