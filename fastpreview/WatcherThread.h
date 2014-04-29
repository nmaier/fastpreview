#pragma once
#include <windows.h>
#include <string>
#include "Thread.h"

class WatcherThread : public Thread
{
  HANDLE hDir_;
  HANDLE hTerm_;
  const HWND hOwner_;
  OVERLAPPED ovl_;

  std::wstring dir_;
  std::wstring shortFile_, longFile_;

  LPVOID not_;

  void Register();

protected:
  virtual DWORD operator()();

public:
  WatcherThread(const std::wstring& file, const HWND aOwner);
  ~WatcherThread();

  void Terminate();
};
