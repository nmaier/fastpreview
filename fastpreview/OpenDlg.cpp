#include "opendlg.h"

#include <tchar.h>
#include <memory>
#include <sstream>

#include "rect.h"
#include "console.h"
#include "functions.h"
#include "Exception.h"

static UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (msg != WM_NOTIFY) {
    return 0;
  }

  NMHDR *hdr = reinterpret_cast<NMHDR*>(lparam);
  if (hdr->code == CDN_INITDONE) {
    Rect rw, wa;
    GetWindowRect(
      hdr->hwndFrom,
      &rw
      );
    SystemParametersInfo(
      SPI_GETWORKAREA,
      0,
      &wa,
      0
      );
    rw.centerIn(wa);

    MoveWindow(
      hdr->hwndFrom,
      rw.left,
      rw.top,
      rw.width(),
      rw.height(),
      TRUE
      );
  }
  return 0;
}

OpenDlg::OpenDlg(const FilterList& aFilter, const std::wstring& aInitialDir)
: filter_(aFilter), initialDir_(aInitialDir)
{
  buf_[0] = '\0';

  lStructSize = sizeof(OPENFILENAME);
  hInstance = nullptr;
  lpstrFile = buf_;
  nMaxFile = 1024;
  Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
    OFN_DONTADDTORECENT | OFN_LONGNAMES | OFN_ENABLESIZING |
    OFN_NONETWORKBUTTON | OFN_ENABLEHOOK;
  FlagsEx = 0;
  lpTemplateName = nullptr;
  lpstrDefExt = lpstrTitle = lpstrInitialDir = nullptr;
  lpstrCustomFilter = lpstrFileTitle = nullptr;
  lpfnHook = OFNHookProc;
}

OpenDlg::~OpenDlg(void)
{}

bool OpenDlg::Execute(HWND hOwner, const std::wstring& aFile)
{
  pvReserved = nullptr;
  dwReserved = 0;
  hwndOwner = hOwner;
  std::unique_ptr<wchar_t[]> filter;

  if (!aFile.empty()) {
    wcscpy_s(lpstrFile, 1024, aFile.c_str());
  }

  if (!filter_.empty()) {
    std::wstringstream ss;
    for (const auto& i : filter_) {
      ss << i.first.c_str() << L'\0' << i.second << L'\0';
    }
    ss << L'\0';
    auto flt = ss.str();
    filter = std::make_unique<wchar_t[]>(flt.length());
    memcpy(filter.get(), flt.c_str(), flt.length() * sizeof(wchar_t));
  }
  lpstrFilter = filter.get();
  lpstrInitialDir = initialDir_.empty() ? nullptr : initialDir_.c_str();

  nFilterIndex = 1;
  bool rv = ::GetOpenFileName(this) == TRUE;
  if (rv) {
    initialDir_ = std::wstring(lpstrFile, nFileOffset);
  }
  return rv;
}

std::wstring OpenDlg::getFileName() const
{
  return std::wstring(buf_);
}

std::wstring OpenDlg::getInitialDir() const
{
  return initialDir_;
}
