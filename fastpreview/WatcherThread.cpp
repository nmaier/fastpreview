#include "watcherthread.h"

#include <memory>

#include "Exception.h"
#include "functions.h"
#include "console.h"
#include "Messages.h"

WatcherThread::WatcherThread(const std::wstring& file, HWND aOwner)
  : Thread(), longFile_(file), hOwner_(aOwner)
{
  ConWrite(_T("Watching init"));
  hTerm_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  if (hTerm_ == INVALID_HANDLE_VALUE) {
    throw WindowsException();
  }

  size_t pos = longFile_.rfind('\\');
  dir_ = longFile_.substr(0, pos + 1);
  longFile_ = longFile_.substr(pos + 1);
  auto buf = std::make_unique<wchar_t[]>(MAX_PATH);
  if (GetShortPathName(
    file.c_str(),
    buf.get(),
    MAX_PATH
    )) {
    shortFile_ = std::wstring(buf.get());
    pos = shortFile_.rfind('\\');
    shortFile_ = shortFile_.substr(pos + 1);
  }

  ConWrite(shortFile_);
  ConWrite(longFile_);
  ConWrite(dir_);

  hDir_ = CreateFile(
    dir_.c_str(),
    GENERIC_READ,
    FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
    nullptr,
    OPEN_EXISTING,
    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
    nullptr
    );

  if (hDir_ == INVALID_HANDLE_VALUE) {
    throw WindowsException();
  }
  not_ = (LPVOID)LocalAlloc(LPTR, 64 * 1024);
  Run();
}

WatcherThread::~WatcherThread()
{
  try {
    Terminate();
    CloseHandle(hTerm_);
    CloseHandle(hDir_);
    LocalFree(not_);
  }
  catch (...) {
  }
}

void WatcherThread::Register()
{
  ReadDirectoryChangesW(
    hDir_,
    not_,
    64 * 1024,
    FALSE,
    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE |
    FILE_NOTIFY_CHANGE_SIZE,
    nullptr,
    &ovl_,
    nullptr
    );

}

DWORD WatcherThread::operator()()
{
  ovl_.hEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);
  Register();
  HANDLE Handles[] = {hTerm_, ovl_.hEvent};
  DWORD R;
  for (;;) {
    R = WaitForMultipleObjects(
      2,
      Handles,
      FALSE,
      INFINITE
      );
    if (R == WAIT_OBJECT_0) {
      break;
    }

    if (R == WAIT_OBJECT_0 + 1) {
      PFILE_NOTIFY_INFORMATION cnot = (PFILE_NOTIFY_INFORMATION)not_;
      DWORD off;
      DWORD msg = 0;
      do {
        std::wstring cmp(cnot->FileName, cnot->FileName + cnot->FileNameLength / sizeof(wchar_t));
        const wchar_t *cmps = cmp.c_str();
        if (!_wcsicmp(cmps, shortFile_.c_str()) ||
          !_wcsicmp(cmps, longFile_.c_str())) {
          msg = cnot->Action;
        }
        off = cnot->NextEntryOffset;
        cnot = (PFILE_NOTIFY_INFORMATION)((LPBYTE)cnot + off);
      } while (off);

      if (msg) {
        PostMessage(hOwner_, WM_WATCH, (WPARAM)msg, 0);
        Sleep(1000);
      }
      Register();
      break;
    }

    MessageBox(hOwner_, _T("Watcher Error"), _T("Error"), 0);
    break;
  }
  if (ovl_.hEvent) {
    CloseHandle(ovl_.hEvent);
  }
  return 0;
}

void WatcherThread::Terminate()
{
  SetEvent(hTerm_);
  WaitForSingleObject(GetHandle(), INFINITE);
}