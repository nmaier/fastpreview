#pragma once

#include <Windows.h>

class __declspec(novtable) Priority
{
  DWORD restore_;

public:
  Priority(DWORD newPriority)
  {
    auto pid = ::GetCurrentProcess();
    restore_ = ::GetPriorityClass(pid);
    ::SetPriorityClass(pid, newPriority);
  }

  ~Priority()
  {
    ::SetPriorityClass(
      ::GetCurrentProcess(),
      restore_ ? restore_ : NORMAL_PRIORITY_CLASS
      );
  }
};