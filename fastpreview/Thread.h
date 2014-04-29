#pragma once

#include "Console.h"

class Thread
{
  friend class Locker;
  friend DWORD WINAPI ThreadProc(LPVOID);

private:
  HANDLE hthread_;
  CRITICAL_SECTION sec_;

protected:
  virtual DWORD operator()() = 0;

public:
  Thread();
  ~Thread();

  void Run()
  {
    ResumeThread(hthread_);
  }

  const HANDLE &GetHandle() const
  {
    return hthread_;
  }

public:
  class Locker
  {
  private:
    CRITICAL_SECTION& sec_;

  public:
    Locker(Thread *owner) : sec_(owner->sec_)
    {
      EnterCriticalSection(&sec_);
    }

    ~Locker()
    {
      LeaveCriticalSection(&sec_);
    }
  };

  bool Suspend()
  {
    ConWrite(_T("Suspend"));
    return SuspendThread(hthread_) != (DWORD)-1;
  }

  class Suspender
  {
  private:
    Thread *thread_;

  public:
    Suspender(Thread *aThread) : thread_(aThread)
    {
      thread_->Suspend();
    }

    ~Suspender()
    {
      thread_->Run();
    }
  };
};
