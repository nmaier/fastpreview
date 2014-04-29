#include "FPShellExt.h"
#include "resource.h"
#include "stringtools.h"

#include <FreeImagePlus.h>
#include <string>
#include <sstream>

namespace {
  static const std::wstring col_name = stringtools::loadResourceString(IDS_COL_NAME);
  static const std::wstring col_desc = stringtools::loadResourceString(IDS_COL_DESC);
  static const std::wstring col_exts = stringtools::loadResourceString(IDS_COL_EXTS);
};

DWORD WINAPI OptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  IFPShellExt *owner = (IFPShellExt*)GetProp(hDlg, L"owner");
  switch (msg) {
  case WM_INITDIALOG: {
    SetProp(hDlg, L"owner", (HANDLE)lParam);
    return TRUE;
  }

  case WM_SHOWWINDOW: {
    if (::GetProp(hDlg, L"INITED")) {
      return FALSE;
    }
    ::SetProp(hDlg, L"INITED", (HANDLE)TRUE);

    // caption
    const int fifcnt = FreeImage_GetFIFCount();
    SetWindowText(
      hDlg,
      stringtools::formatResourceString(IDS_SUPPORTEDFMT, fifcnt).c_str()
      );

    std::wstringstream ss;
    ss << L"FastPreview » Shell Extension..." << std::endl
      << L"© 2004-2014 by Nils Maier" << std::endl
      << std::endl
      << stringtools::convert(FreeImage_GetCopyrightMessage()) << std::endl;

    // about
    SetDlgItemText(
      hDlg,
      IDC_ABOUT,
      ss.str().c_str()
      );

    // formats
    HWND ctrl = GetDlgItem(hDlg, IDC_FORMATS);

    LVCOLUMN column;
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    column.fmt = LVCFMT_LEFT;
    column.cx = 100;

    column.pszText = const_cast<wchar_t*>(col_name.c_str());
    ListView_InsertColumn(ctrl, 0, &column);

    column.pszText = const_cast<wchar_t*>(col_desc.c_str());
    ListView_InsertColumn(ctrl, 1, &column);

    column.pszText = const_cast<wchar_t*>(col_exts.c_str());
    ListView_InsertColumn(ctrl, 2, &column);

    LVITEM item;

    auto text = std::make_unique<wchar_t[]>(260);
    item.pszText = text.get();
    item.pszText[259] = '\0';

    item.mask = LVIF_TEXT;

    for (int i = 0; i < fifcnt; ++i) {
      FreeImage::Format fmt((FREE_IMAGE_FORMAT)i);
      if (!fmt.isValid()) {
        continue;
      }

      item.iItem = item.iSubItem = 0;
      (void)lstrcpynW(
        item.pszText,
        stringtools::convert(fmt.getName()).c_str(),
        259
        );
      item.iItem = ListView_InsertItem(ctrl, &item);

      ++item.iSubItem;
      (void)lstrcpynW(
        item.pszText,
        stringtools::convert(fmt.getDescription()).c_str(),
        259
        );
      ListView_SetItem(ctrl, &item);

      ++item.iSubItem;
      (void)lstrcpynW(
        item.pszText,
        stringtools::convert(fmt.getExtensions()).c_str(),
        259
        );
      ListView_SetItem(ctrl, &item);
    }
    for (int i = 0; i < 3; ++i)
      ListView_SetColumnWidth(ctrl, i, LVSCW_AUTOSIZE);

    // updowns
    ctrl = GetDlgItem(hDlg, IDC_WIDTH);
    SendMessage(
      ctrl,
      UDM_SETRANGE32,
      (WPARAM)80,
      (LPARAM)500
      );
    SendMessage(
      ctrl,
      UDM_SETBUDDY,
      (WPARAM)GetDlgItem(hDlg, IDC_WIDTHT),
      0
      );
    SendMessage(
      ctrl,
      UDM_SETPOS32,
      0,
      (LPARAM)owner->height_
      );


    ctrl = GetDlgItem(hDlg, IDC_HEIGHT);
    SendMessage(
      ctrl,
      UDM_SETRANGE32,
      (WPARAM)80,
      (LPARAM)500
      );
    SendMessage(
      ctrl,
      UDM_SETBUDDY,
      (WPARAM)GetDlgItem(hDlg, IDC_HEIGHTT),
      0
      );
    SendMessage(
      ctrl,
      UDM_SETPOS32,
      0,
      (LPARAM)owner->width_
      );

    // opts
    CheckDlgButton(
      hDlg,
      IDC_CHECK_SHOW,
      owner->showThumb_ ? BST_CHECKED : BST_UNCHECKED
      );
    return TRUE;
  }

  case WM_COMMAND: {
    switch (wParam) {
    case IDOK: {
      // Width/Height
      BOOL error = TRUE;
      uint32_t value = (uint32_t)SendDlgItemMessage(
        hDlg,
        IDC_WIDTH,
        UDM_GETPOS32,
        0,
        (LPARAM)&error
        );
      if (!error) {
        owner->reg_.set(L"extThumbWidth", value);
      }

      error = TRUE;
      value = (uint32_t)SendDlgItemMessage(
        hDlg,
        IDC_HEIGHT,
        UDM_GETPOS32,
        0,
        (LPARAM)&error
        );
      if (!error) {
        owner->reg_.set(L"extThumbHeight", value);
      }

      // opts
      owner->reg_.set(
        L"extShowThumb",
        IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW) ? true : false
        );

      EndDialog(hDlg, TRUE);
    }
      return TRUE;

    case IDCANCEL:
      EndDialog(hDlg, FALSE);
      return TRUE;
    }
  }
    break;
  }

  return FALSE;
}