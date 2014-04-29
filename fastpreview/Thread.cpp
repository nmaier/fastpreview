#include <windows.h>
#include "Thread.h"
#include "Exception.h"

DWORD WINAPI ThreadProc(LPVOID data)
{
  return (*reinterpret_cast<Thread*>(data))();
}

Thread::Thread() : hthread_(nullptr)
{
  InitializeCriticalSectionAndSpinCount(&sec_, 3000);

  hthread_ = ::CreateThread(
    nullptr, 0, ThreadProc, this, CREATE_SUSPENDED, nullptr);
  if (hthread_ == INVALID_HANDLE_VALUE) {
    throw WindowsException();
  }
}

Thread::~Thread()
{
  CloseHandle(hthread_);
  DeleteCriticalSection(&sec_);
}