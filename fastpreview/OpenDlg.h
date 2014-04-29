#pragma once

#include <windows.h>
#include <tchar.h>

#include <string>
#include <map>
#include <deque>

class OpenDlg : OPENFILENAME
{
public:
  typedef std::pair<std::wstring, std::wstring> FilterType;
  typedef std::deque<FilterType> FilterList;

private:
  wchar_t buf_[1024];
  FilterList filter_;
  std::wstring initialDir_;

public:
  OpenDlg(const FilterList& aFilter, const std::wstring& aInitialDir);
  ~OpenDlg(void);

  bool Execute(HWND hOwner = nullptr, const std::wstring& aFile = _T(""));

  std::wstring getFileName() const;
  std::wstring getInitialDir() const;
};
