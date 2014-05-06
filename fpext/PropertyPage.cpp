#include "PropertyPage.h"

#include <sstream>
#include <strsafe.h>

#include "ShellExt.h"
#include "ComServers.h"
#include "stringtools.h"
#include "resource.h"
#include "TinyXMP.h""

typedef struct _secstruct_t
{
  std::wstring title;
  FREE_IMAGE_MDMODEL model;
} secstruct_t;

static secstruct_t sections[] = {
  { L"Comments", FIMD_COMMENTS },
  { L"IPTC/NAA", FIMD_IPTC },
  { L"Exif", FIMD_EXIF_EXIF },
  { L"Exif Main", FIMD_EXIF_MAIN },
  { L"Exif GPS", FIMD_EXIF_GPS },
  { L"GeoTIFF", FIMD_GEOTIFF },
  { L"Adobe XMP", FIMD_XMP },
  { L"Animation", FIMD_ANIMATION },
  { L"MarkerNotes", FIMD_EXIF_MAKERNOTE },
  { L"Exif Interop", FIMD_EXIF_INTEROP },
};

const std::wstring PropertyPage::title = stringtools::loadResourceString(IDS_FASTPREVIEW);
const std::wstring PropertyPage::col_type = stringtools::loadResourceString(IDS_COL_TYPE);
const std::wstring PropertyPage::col_value = stringtools::loadResourceString(IDS_COL_VALUE);

PropertyPage::PropertyPage(
  ShellExt *ext, const std::wstring& aFile, UINT& ref)
  : file_(aFile), hwnd_(nullptr), hlist_(nullptr), ext_(ext)
{
  if (!img_.load(file_) || !img_.makeThumbnail(96, 96)) {
    throw std::wstring(L"failed to load image");
  }

  PROPSHEETPAGE psp;
  ZeroMemory(&psp, sizeof(PROPSHEETPAGE));

  psp.dwSize = sizeof(psp);
  psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEHICON |
    PSP_USECALLBACK;

  psp.hInstance = gModThis;
  psp.pcRefParent = &ref;
  psp.pfnDlgProc = proc;
  psp.pfnCallback = callbackProc;
  psp.lParam = reinterpret_cast<LPARAM>(this);

  psp.pszTitle = title.c_str();
  psp.pszTemplate = MAKEINTRESOURCE(IDD_FPPROPPAGE);
  psp.hIcon = (HICON)::LoadImage(
    COMServers::GetModuleHandle(),
    MAKEINTRESOURCE(IDI_APP),
    IMAGE_ICON,
    ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON),
    LR_LOADTRANSPARENT | LR_SHARED
    );

  handle_ = ::CreatePropertySheetPage(&psp);
  if (!handle_) {
    throw std::wstring(L"failed to create prop page");
  }

  if (ext_) {
    ext_->AddRef();
  }
}

PropertyPage::~PropertyPage()
{
  if (ext_) {
    ext_->Release();
    ext_ = nullptr;
  }
}

#define PAGE_PTR L"FPPropPage"
INT_PTR CALLBACK PropertyPage::proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  PropertyPage *page = nullptr;

  if (msg == WM_INITDIALOG) {
    page = reinterpret_cast<PropertyPage*>(reinterpret_cast<PROPSHEETPAGE*>(lparam)->lParam);
    page->hwnd_ = hwnd;
    ::SetProp(hwnd, PAGE_PTR, (HANDLE)page);
  }
  else {
    page = reinterpret_cast<PropertyPage*>(::GetProp(hwnd, PAGE_PTR));
  }

  if (page != 0) {
    return page->loop(msg, wparam, lparam);
  }

  return FALSE;
}

UINT CALLBACK PropertyPage::callbackProc(HWND hwnd, UINT msg, LPPROPSHEETPAGE lparam)
{
  if (msg == PSPCB_RELEASE) {
    ::RemoveProp(hwnd, PAGE_PTR);
    delete reinterpret_cast<PropertyPage*>(lparam->lParam);
    lparam->lParam = 0;
  }
  return 1;
}

INT_PTR PropertyPage::loop(UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
  case WM_INITDIALOG:
    init();
    return TRUE;
  case WM_DRAWITEM:
    switch (wparam) {
    case IDC_THUMB:
      drawImg(reinterpret_cast<LPDRAWITEMSTRUCT>(lparam));
      return TRUE;
    }
    break;
  case WM_CONTEXTMENU: {
    if (GetDlgItem(hwnd_, IDC_DATA) != reinterpret_cast<HWND>(wparam) ||
      !ListView_GetItemCount(hlist_)) {
      break;
    }
    HMENU hmenu = LoadMenu(gModThis, MAKEINTRESOURCE(IDR_DATA_CTX));
    HMENU hctx = GetSubMenu(hmenu, 0);

    const UINT flag = MF_BYCOMMAND |
      (ListView_GetSelectionMark(hlist_) == -1 ? MF_GRAYED : MF_ENABLED);

    EnableMenuItem(
      hctx,
      ID_DATACTX_COPY,
      flag
      );
    EnableMenuItem(
      hctx,
      ID_DATACTX_COPYDATA,
      flag
      );
    EnableMenuItem(
      hctx,
      ID_DATACTX_COPYKEY,
      flag
      );

    TrackPopupMenu(
      hctx,
      0,
      LOWORD(lparam),
      HIWORD(lparam),
      0,
      hwnd_,
      nullptr
      );
    DestroyMenu(hmenu);
    return TRUE;
  }

  case WM_COMMAND:
    handleCommand(wparam, lparam);
    return TRUE;

  case WM_LBUTTONDBLCLK: {
    POINT pt;
    pt.x = LOWORD(lparam);
    pt.y = HIWORD(lparam);
    RECT cr;
    RECT wr;
    if (GetWindowRect(GetDlgItem(hwnd_, IDC_THUMB), &cr) &&
      GetWindowRect(hwnd_, &wr) && OffsetRect(&cr, -wr.left, -wr.top) &&
      PtInRect(&cr, pt)) {
      ShellExt::createView(hwnd_, file_);
      return TRUE;
    }
    break;
  }
  } // switch

  return FALSE;
}

namespace {
  static std::wstring sanitize(const std::wstring& in)
  {
    std::wstringstream ss;
    bool lastIsWhitespace = true;
    for (const auto& ch : in) {
      switch (ch) {
      case ' ':
      case '\t':
      case '\r':
      case '\v':
      case '\n':
        if (!lastIsWhitespace) {
          ss << ' ';
        }
        break;

      default:
        lastIsWhitespace = false;
        ss << ch;
      }
    }
    return ss.str();
  }
}

void PropertyPage::init()
{
  SetDlgItemText(hwnd_, IDC_FILE, file_.c_str());

  const FreeImage::Information& info = img_.getOriginalInformation();

  auto typestr = stringtools::formatResourceString(
    IDS_PROP_TYPE,
    stringtools::convert(info.getFormat().getName()).c_str(),
    info.getBitsPerPixel()
    );
  SetDlgItemText(
    hwnd_,
    IDC_TYPE,
    typestr.c_str()
    );

  wchar_t mp[20];
  StringCchPrintf(mp, 20, L"%.2f",
    (double)info.getWidth() * info.getHeight() / 1000.0 / 1000.0);
  auto dimstr = stringtools::formatResourceString(
    IDS_PROP_DIMS, info.getWidth(), info.getHeight(), mp);
  SetDlgItemText(
    hwnd_,
    IDC_DIMS,
    dimstr.c_str()
    );


  hlist_ = reinterpret_cast<HWND>(GetDlgItem(hwnd_, IDC_DATA));

  LVCOLUMN column;
  column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
  column.fmt = LVCFMT_LEFT;
  column.cx = 100;

  column.pszText = const_cast<wchar_t*>(col_type.c_str());
  ListView_InsertColumn(hlist_, 0, &column);

  column.pszText = const_cast<wchar_t*>(col_value.c_str());
  ListView_InsertColumn(hlist_, 1, &column);

  ListView_EnableGroupView(hlist_, TRUE);

  LVGROUP group;
  ZeroMemory(&group, sizeof(group));
  group.cbSize = sizeof(group);
  group.iGroupId = 100;
  group.mask = LVGF_ALIGN | LVGF_HEADER | LVGF_GROUPID;
  group.uAlign = LVGA_HEADER_LEFT;

  LVITEM item;
  ZeroMemory(&item, sizeof(item));
  item.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_COLUMNS;

  FreeImage::Tag tag;
  if (img_.getMetadata(FIMD_XMP, "XMLPacket", tag)) {
    std::string xml(reinterpret_cast<const char*>(tag.getValue()));
    try {
      TinyXMP::XMP xmp;
      xmp.parse(xml);
      for (const auto& i : xmp.getNamespaces()) {
        std::wstring desc(L"XMP ");
        desc.append(stringtools::convert(i.getDescription()));
        group.pszHeader = const_cast<LPWSTR>(desc.c_str());
        group.cchHeader = (int)desc.length();
        ListView_InsertGroup(hlist_, -1, &group);

        item.cColumns = 0;
        item.iGroupId = group.iGroupId++;
        item.iItem = 0;

        for (const auto& ii : i.getEntries()) {
          for (const auto& iii : ii.second) {
            std::wstring value = sanitize(stringtools::convert(iii, CP_UTF8));
            if (value.empty()) {
              continue;
            }
            std::wstring key = sanitize(stringtools::convert(ii.first));

            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(key.c_str());
            item.iItem = ListView_InsertItem(hlist_, &item);
            ListView_SetItemText(
              hlist_,
              item.iItem,
              ++item.iSubItem,
              const_cast<LPWSTR>(value.c_str())
              );
          }
        }
      }
    }
    catch (const TinyXMP::Exception&) {
      // no op
    }
  }
  for (const auto& sec : sections) {
    FreeImage::MetadataFind finder;
    FreeImage::Tag tag;
    if (!finder.findFirstMetadata(sec.model, img_, tag)) {
      continue;
    }
    if (!tag.isValid()) {
      continue;
    }

    group.pszHeader = const_cast<LPWSTR>(sec.title.c_str());
    group.cchHeader = (int)sec.title.length();
    ListView_InsertGroup(hlist_, -1, &group);

    item.cColumns = 0;
    item.iGroupId = group.iGroupId++;
    item.iItem = 0;

    while (finder.findNextMetadata(tag)) {
      if (!tag.isValid()) {
        dump(L"Invalid tag");
        continue;
      }
      std::wstring value = sanitize(
        stringtools::convert(tag.toString(sec.model)));
      if (value.empty()) {
        continue;
      }
      std::wstring key = stringtools::convert(tag.getDescription());
      if (key.empty()) {
        key = stringtools::convert(tag.getKey());
      }
      key = sanitize(key);

      item.iSubItem = 0;
      item.pszText = const_cast<LPWSTR>(key.c_str());
      item.iItem = ListView_InsertItem(hlist_, &item);
      ListView_SetItemText(
        hlist_,
        item.iItem,
        ++item.iSubItem,
        const_cast<LPWSTR>(value.c_str())
        );
    }
  }
  if (ListView_GetItemCount(hlist_) != 0) {
    ListView_SetColumnWidth(hlist_, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hlist_, 1, LVSCW_AUTOSIZE);
    int w = ListView_GetColumnWidth(hlist_, 0);
    if (w > 140) {
      ListView_SetColumnWidth(hlist_, 0, 140);
    }
  }

  LONG_PTR style = ListView_GetExtendedListViewStyle(hlist_);
  ListView_SetExtendedListViewStyle(hlist_, style | LVS_EX_FULLROWSELECT);
}

static void putIntoClipboard(HWND hwnd, const std::wstring& text)
{
  char *ansi;
  wchar_t *unicode;

  if (OpenClipboard(hwnd) == FALSE) {
    return;
  }

  const auto len = text.length() + 1;

  EmptyClipboard();

  HGLOBAL hdata = GlobalAlloc(GHND, len * sizeof(wchar_t));
  if (hdata == 0) {
    goto exit;
  }

  unicode = reinterpret_cast<wchar_t*>(GlobalLock(hdata));
  if (unicode == 0) {
    GlobalFree(hdata);
    goto exit;
  }

  wcscpy_s(unicode, len, text.c_str());
  GlobalUnlock(hdata);
  SetClipboardData(CF_UNICODETEXT, hdata);

  hdata = GlobalAlloc(GHND, len * sizeof(char));
  if (hdata == 0) {
    goto exit;
  }
  ansi = reinterpret_cast<char*>(GlobalLock(hdata));
  if (ansi == 0) {
    GlobalFree(hdata);
    goto exit;
  }
  strcpy_s(ansi, len, stringtools::WStringToString(text).c_str());
  GlobalUnlock(hdata);
  SetClipboardData(CF_TEXT, hdata);

exit:
  CloseClipboard();
}

void PropertyPage::handleCommand(WPARAM wparam, LPARAM lparam)
{
  const int idx = ListView_GetSelectionMark(hlist_);
  std::wstringstream ss;

  auto buf = std::make_unique<wchar_t[]>(10240);
  ZeroMemory(buf.get(), 10240);

  auto lowparam = LOWORD(wparam);
  switch (lowparam) {
  case ID_DATACTX_COPY:
    if (idx == -1) {
      return;
    }
    ListView_GetItemText(hlist_, idx, 0, buf.get(), 102399);
    ss << buf.get() << L" - ";
    ListView_GetItemText(hlist_, idx, 1, buf.get(), 102399);
    ss << buf.get();
    putIntoClipboard(hwnd_, ss.str());
    break;

  case IDC_BUTTON_COPYALL:
  case ID_DATACTX_COPYALL: {
    int lastGid = -1;
    LVITEM item;
    ZeroMemory(&item, sizeof(item));
    item.pszText = buf.get();
    item.cchTextMax = 102399;
    item.mask = LVIF_TEXT | LVIF_GROUPID;
    for (int i = 0, e = ListView_GetItemCount(hlist_); i < e; ++i) {
      item.iItem = e - i - 1;
      item.iSubItem = 0;

      ListView_GetItem(hlist_, &item);
      std::wstring key = buf.get();

      if (item.iGroupId != lastGid) {
        if (lastGid != -1) {
          ss << std::endl << std::endl;
        }
        lastGid = item.iGroupId;
        LVGROUP group;
        group.cbSize = sizeof(group);
        group.cchHeader = 102399;
        group.pszHeader = buf.get();
        group.mask = LVGF_HEADER;
        ListView_GetGroupInfo(hlist_, item.iGroupId, &group);
        ss << L"= " << group.pszHeader << L" =" << std::endl;
      }

      ss << key << L" - ";
      item.iSubItem = 1;
      ListView_GetItem(hlist_, &item);
      ss << item.pszText << std::endl;
    }
    putIntoClipboard(hwnd_, ss.str());
    break;
  }

  case ID_DATACTX_COPYDATA:
    if (idx == -1) {
      return;
    }
    ListView_GetItemText(hlist_, idx, 1, buf.get(), 102399);
    ss << buf.get();
    putIntoClipboard(hwnd_, ss.str());
    break;

  case ID_DATACTX_COPYKEY:
    if (idx == -1) {
      return;
    }
    ListView_GetItemText(hlist_, idx, 0, buf.get(), 102399);
    ss << buf.get();
    putIntoClipboard(hwnd_, ss.str());
    break;

  case IDC_BUTTON_OPTIONS: {
    if (ext_ && ::DialogBoxParam(
      COMServers::GetModuleHandle(),
      MAKEINTRESOURCE(IDD_OPTIONS),
      hwnd_,
      (DLGPROC)OptionsDlgProc,
      (LPARAM)ext_
      )) {
      ext_->initializeFromRegistry();
    }
    break;
  }

  } // switch
}

void PropertyPage::drawImg(LPDRAWITEMSTRUCT dis)
{
  if (dis == 0) {
    return;
  }
  RECT& r = dis->rcItem;

  DrawFrameControl(
    dis->hDC, &r, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_FLAT | DFCS_TRANSPARENT);

  const int w = img_.getWidth();
  const int h = img_.getHeight();
  const int cw = ((r.right - r.left) - w) / 2 - 4;
  const int ch = ((r.bottom - r.top) - h) / 2 - 4;
  r.left += cw + 2;
  r.right -= cw + 2;
  r.top += ch + 2;
  r.bottom -= ch + 2;
  img_.draw(dis->hDC, r);
}