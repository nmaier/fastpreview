#include "ShellExt.h"

#include <stdio.h>
#include <strsafe.h>
#include <shellapi.h>
#include <sstream>

#include "ComServers.h"
#include "stringtools.h"
#include "resource.h"
#include "PropertyPage.h"
#include "Registry.h"

namespace {
  static const std::wstring s_error = stringtools::loadResourceString(IDS_ERROR);
  static const std::wstring s_infosnsettings = stringtools::loadResourceString(IDS_INFOSNSETTINGS);
  static const std::wstring s_thisfile = stringtools::loadResourceString(IDS_THISFILE);
  static const std::wstring s_fastpreview = stringtools::loadResourceString(IDS_FASTPREVIEW);
};

UINT ShellExt::grefcnt = 0;

ShellExt::ShellExt()
: refcnt_(0), reg_(HKEY_CURRENT_USER, L"Software\\MaierSoft\\FastPreview"), alpha_(WTSAT_UNKNOWN),
width_(300), height_(300), maxSize_(1 << 25), showThumb_(true)
{
  (void)::CoInitialize(nullptr);
  ::InterlockedIncrement(&grefcnt);
  ++grefcnt;

  reg_.create();
  initializeFromRegistry();

  INITCOMMONCONTROLSEX cc = {
    sizeof(INITCOMMONCONTROLSEX),
    ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES | ICC_TAB_CLASSES | ICC_DATE_CLASSES
  };
  ::InitCommonControlsEx(&cc);
}

void ShellExt::initializeFromRegistry()
{
  uint32_t tmp = 0;
  if (reg_.get(L"extThumbWidth", tmp) && tmp > 32 && tmp < 2000) {
    width_ = tmp;
  }
  if (reg_.get(L"extThumbHeight", tmp) && tmp > 32 && tmp < 2000) {
    height_ = tmp;
  }
  reg_.get(L"extShowThumb", showThumb_);
  if (reg_.get(L"extMaxSize", tmp) && tmp >= (1 << 16)) {
    maxSize_ = tmp;
  }
}

ShellExt::~ShellExt()
{
  ::InterlockedDecrement(&grefcnt);
  ::CoUninitialize();
}

IFACEMETHODIMP ShellExt::QueryInterface(REFIID iid, void ** ppv)
{
  if (!ppv) {
    return E_INVALIDARG;
  }
  *ppv = nullptr;

  if (IsEqualIID(iid, IID_IUnknown) && (*ppv = this) != nullptr) {
    dump(L"Fullfilled by %s", L"IUnknown");
    AddRef();
    return S_OK;
  }

#define IFACE(iface) \
  if (IsEqualIID(iid, IID_##iface) && (*ppv = static_cast<iface *>(this)) != nullptr) { dump(L"Fullfilled by %s", stringtools::convert(#iface).c_str()); AddRef(); return S_OK; }

  IFACE(IShellExtInit);
  IFACE(IContextMenu);
  IFACE(IContextMenu2);
  IFACE(IContextMenu3);
  IFACE(IShellPropSheetExt);
  IFACE(IThumbnailProvider);
  IFACE(IInitializeWithItem);
  IFACE(IInitializeWithStream);
  IFACE(IInitializeWithFile);
#undef IFACE

  dump(L"No Interface");
  return E_NOINTERFACE;
}

ULONG __stdcall ShellExt::AddRef(void)
{
  return ::InterlockedIncrement((LONG *)&refcnt_);

}

ULONG __stdcall ShellExt::Release(void)
{
  if (!::InterlockedDecrement((LONG *)&refcnt_)) {
    delete this;
    return 0;
  }
  return refcnt_;
}

// IShellExtInit
IFACEMETHODIMP ShellExt::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pDataObj, HKEY hkeyProgID)
{
  image_.clear();

  FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  STGMEDIUM stg = { TYMED_HGLOBAL };
  HDROP drop;

  // Look for CF_HDROP data in the data object.
  if (FAILED(pDataObj->GetData(&fmt, &stg))) {
    return E_INVALIDARG;
  }
  std::unique_ptr<
    std::remove_pointer<HANDLE>::type,
    decltype(&::GlobalUnlock)
  > sgguard(stg.hGlobal, ::GlobalUnlock);
  std::unique_ptr<
    STGMEDIUM,
    decltype(&::ReleaseStgMedium)
  > stgguard(&stg, ::ReleaseStgMedium);

  // Get a pointer to the actual data.
  drop = (HDROP)GlobalLock(stg.hGlobal);

  // Make sure it worked.
  if (!drop) {
    return E_INVALIDARG;
  }

  // Sanity check - make sure there is at least one filename.
  UINT uNumFiles = DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);

  wchar_t path[MAX_PATH];

  if (!uNumFiles) {
    return E_INVALIDARG;
  }
  if (uNumFiles > 5) {
    return E_INVALIDARG;
  }
  if (!DragQueryFile(drop, 0, path, MAX_PATH)) {
    return E_INVALIDARG;
  }

  path_ = path;
  return S_OK;
}

// IInitializeWithItem
IFACEMETHODIMP ShellExt::Initialize(IShellItem *psi, DWORD grfMode)
{
  dump(L"InitializeWithItem");
  image_.clear();

  if (!psi) {
    return E_POINTER;
  }

  wchar_t *file = nullptr;
  auto rv = psi->GetDisplayName(SIGDN_FILESYSPATH, &file);
  if (FAILED(rv)) {
    dump(L"InitializeWithItem - NoName");
    return rv;
  }
  if (!file) {
    dump(L"InitializeWithItem - NoName2");
    throw E_POINTER;
  }

  path_ = file;
  dump(L"InitializeWithItem - File: %s", path_.c_str());
  ::CoTaskMemFree(file);
  return S_OK;
}

// IntialzeWithFile
IFACEMETHODIMP ShellExt::Initialize(LPCWSTR pszFilePath, DWORD grfMode)
{
  if (!pszFilePath) {
    return E_INVALIDARG;
  }

  path_ = pszFilePath;
  return S_OK;
}

// InitializeWithStream
IFACEMETHODIMP ShellExt::Initialize(IStream *pstream, DWORD grfMode)
{
  if (!pstream) {
    return E_INVALIDARG;
  }

  stream_.reset(pstream);
  stream_->AddRef();
  return S_OK;
}

// IContextMenu
IFACEMETHODIMP ShellExt::GetCommandString(
  UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
  switch (uFlags) {
  case GCS_HELPTEXTW:
    switch (idCmd) {
    case 2:
      wcscpy_s((wchar_t*)pszName, cchMax, s_infosnsettings.c_str());
      break;
    case 3:
      wcscpy_s((wchar_t*)pszName, cchMax, s_thisfile.c_str());
      break;
    default:
      wcscpy_s((wchar_t*)pszName, cchMax, s_fastpreview.c_str());
      break;
    }
    return NOERROR;

  case GCS_VALIDATEW:
    return (idCmd == 3 || idCmd == 2) ? S_OK : S_FALSE;
  }
  return E_NOTIMPL;
}

void ShellExt::createView(HWND hwnd, const std::wstring& path)
{
  auto prog = COMServers::ModuleDirectory();
  prog.append(L"fastpreview.exe");

  std::wstring file(L"\"");
  file.append(path);
  file.append(L"\"");

  SHELLEXECUTEINFO sei;
  sei.cbSize = sizeof(SHELLEXECUTEINFO);
  sei.fMask =
    SEE_MASK_NOZONECHECKS | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE |
    SEE_MASK_HMONITOR;
  sei.hMonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  sei.hwnd = hwnd;
  sei.lpFile = prog.c_str();
  sei.lpParameters = file.c_str();
  sei.lpDirectory = nullptr;
  sei.lpVerb = nullptr;
  sei.nShow = SW_SHOW;

  ::ShellExecuteEx(&sei);
}

namespace {
  static __int64 __fastcall getFileSize(const std::wstring& path)
  {
    __int64 fs = 0;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &fad)) {
      fs = fad.nFileSizeHigh * MAXDWORD + fad.nFileSizeLow;
    }
    return fs;
  }
}

void ShellExt::showOptions(HWND hWnd)
{
  auto rv = DialogBoxParam(
    COMServers::GetModuleHandle(),
    MAKEINTRESOURCE(IDD_OPTIONS),
    hWnd,
    (DLGPROC)OptionsDlgProc,
    (LPARAM)this
    );
  if (rv) {
    initializeFromRegistry();
  }
}

IFACEMETHODIMP ShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO pCmdInfo)
{
  if (HIWORD(pCmdInfo->lpVerb)) {
    return E_INVALIDARG;
  }

  switch (LOWORD(pCmdInfo->lpVerb)) {
  case 3:
    createView(pCmdInfo->hwnd, path_);
    return S_OK;

  case 2:
    showOptions(pCmdInfo->hwnd);
    return S_OK;

  default:
    return E_INVALIDARG;
  }
}

IFACEMETHODIMP ShellExt::QueryContextMenu(
  HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
  if (!showThumb_) {
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
  }

  if ((uFlags & CMF_DEFAULTONLY) || (uFlags & CMF_NOVERBS) ||
    getFileSize(path_) > maxSize_) {
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
  }

  if (!image_.load(path_)) {
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
  }
  info_ = FreeImage::StaticInformation(image_.getOriginalInformation());

  try {
    image_.rescale(width_, height_, FILTER_CATMULLROM, true);
  }
  catch (std::exception) {
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
  }
  catch (...) {
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
  }

  ::InsertMenu(
    hmenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, ++idCmdFirst, L""
    );

  ::InsertMenu(
    hmenu, indexMenu, MF_BYPOSITION | MF_STRING, ++idCmdFirst,
    s_infosnsettings.c_str()
    );

  wchar_t mp[20];
  StringCchPrintf(mp, 20, L"%.2f",
    (double)info_.getWidth() * info_.getHeight() / 1000.0 / 1000.0);
  auto infostr = stringtools::formatResourceString(
    IDS_INFOSTR, info_.getWidth(), info_.getHeight(),
    stringtools::convert(info_.getFormat().getName()).c_str(),
    info_.getBitsPerPixel(),
    mp
    );
  ::InsertMenu(
    hmenu, indexMenu, MF_BYPOSITION | MF_OWNERDRAW, ++idCmdFirst,
    infostr.c_str()
    );

  return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, USHORT(4));
}

// IContextMenu2
IFACEMETHODIMP ShellExt::HandleMenuMsg(
  UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  return HandleMenuMsg2(uMsg, wParam, lParam, nullptr);
}

// IContextMenu2
IFACEMETHODIMP ShellExt::HandleMenuMsg2(
  UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
#define SR(x) if (lpResult) *lpResult = x;
  try {
    switch (uMsg) {
    case WM_MEASUREITEM: {
      if (!image_.isValid()) {
        SR(FALSE);
        return E_FAIL;
      }

      LPMEASUREITEMSTRUCT mis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
      mis->CtlType = ODT_MENU;
      mis->itemID = 1;
      mis->itemWidth = image_.getWidth() + 10;
      mis->itemHeight = image_.getHeight() + 30;

      SR(TRUE);
      return S_OK;
    }

    case WM_DRAWITEM: {
      if (!image_.isValid()) {
        SR(FALSE);
        return E_FAIL;
      }
      LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
      SetBkMode(dis->hDC, TRANSPARENT);
      if (dis->itemState & ODS_SELECTED) {
        FillRect(
          dis->hDC,
          &dis->rcItem,
          (HBRUSH)(COLOR_MENUHILIGHT + 1)
          );
        SetTextColor(
          dis->hDC,
          GetSysColor(COLOR_HIGHLIGHTTEXT)
          );
      }
      else {
        FillRect(
          dis->hDC,
          &dis->rcItem,
          (HBRUSH)(COLOR_MENU + 1)
          );
        SetTextColor(
          dis->hDC,
          GetSysColor(COLOR_MENUTEXT)
          );
      }

      WORD w = image_.getWidth(), h = image_.getHeight();

      RECT r;
      r.left = dis->rcItem.left + (dis->rcItem.right - dis->rcItem.left - w) / 2;
      r.top = dis->rcItem.top + (dis->rcItem.bottom - (dis->rcItem.top + 30) - h) / 2 + 5;
      r.right = r.left + w;
      r.bottom = r.top + h;

      image_.draw(dis->hDC, r);

      ::DrawEdge(
        dis->hDC, &r,
        (dis->itemState & ODS_SELECTED) ? EDGE_SUNKEN : EDGE_ETCHED, BF_RECT
        );

      memcpy(&r, &dis->rcItem, sizeof(RECT));
      r.top = r.bottom - 25;
      wchar_t mp[20];
      StringCchPrintf(mp, 20, L"%.2f",
        (double)info_.getWidth() * info_.getHeight() / 1000.0 / 1000.0);
      auto infostr = stringtools::formatResourceString(
        IDS_INFOSTR, info_.getWidth(), info_.getHeight(),
        stringtools::convert(info_.getFormat().getName()).c_str(),
        info_.getBitsPerPixel(),
        mp
        );
      ::DrawText(
        dis->hDC, infostr.c_str(), -1, &r,
        DT_SINGLELINE | DT_VCENTER | DT_CENTER
        );
    }
      SR(TRUE);
      return S_OK;
    }
  }
  catch (...) {
    dump(L"Shit happened!");
  }
  SR(FALSE);
  return E_FAIL;
}

IFACEMETHODIMP ShellExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
  try {
    lpfnAddPage((new PropertyPage(this, path_, grefcnt))->getHandle(), lParam);
  }
  catch (const std::wstring& ex) {
#ifdef _DEBUG
    ::MessageBox(nullptr, ex.c_str(), s_error.c_str(), MB_ICONERROR);
#endif
    dump(L"%s", ex);
  }
  catch (...) {
    // no op
  }

  return S_OK;
}

IFACEMETHODIMP ShellExt::ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
{
  return E_NOTIMPL;
}

namespace {
  unsigned fio_read(void *buffer, unsigned size, unsigned count, fi_handle handle)
  {
    char* buf = (char*)buffer;
    IStream* s = (IStream*)handle;
    ULONG read = 0;
    for (; count; --count) {
      ULONG rv = 0;
      if (FAILED(s->Read(buf, size, &rv))) {
        dump(L"FIO - failed read %d", size);
        break;
      }
      read++;
      buf += size;
    }
    return read;
  }
  unsigned fio_write(
    void *buffer, unsigned size, unsigned count, fi_handle handle)
  {
    return 0;
  }
  int fio_seek(fi_handle handle, long offset, int origin)
  {
    IStream* s = (IStream*)handle;
    LARGE_INTEGER o;
    o.QuadPart = offset;
    auto sorigin = origin == SEEK_CUR ? STREAM_SEEK_CUR :
      (origin == SEEK_END ? STREAM_SEEK_END : STREAM_SEEK_SET);

    if (FAILED(s->Seek(o, sorigin, nullptr))) {
      dump(L"FIO - failed seek %d/%d", offset, origin);
      return -1;
    }
    return 0;
  }
  long fio_tell(fi_handle handle)
  {
    IStream* s = (IStream*)handle;
    LARGE_INTEGER o;
    o.QuadPart = 0;
    ULARGE_INTEGER rv;
    if (FAILED(s->Seek(o, STREAM_SEEK_CUR, &rv))) {
      dump(L"FIO - failed tell");
      return -1;
    }
    return (long)rv.QuadPart;
  }

  static FreeImageIO io = {
    fio_read,
    fio_write,
    fio_seek,
    fio_tell
  };
}

IFACEMETHODIMP ShellExt::GetThumbnail(
  UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
  dump(L"GetThumbnail");
  if (!phbmp) {
    return E_POINTER;
  }
  if (path_.empty() && !stream_) {
    return E_NOT_SET;
  }

  *phbmp = nullptr;
  *pdwAlpha = WTSAT_RGB;
  FreeImage::WinImage img;

  if (stream_) {
    dump(L"GetThumbnail - ft");
    auto ft = FreeImage_GetFileTypeFromHandle(&io, stream_.get());
    dump(L"GetThumbnail - ft is %d", ft);

    dump(L"GetThumbnail - Reading from stream");
    img = FreeImage_LoadFromHandle(ft, &io, stream_.get());
    if (!img.isValid()) {
      dump(L"GetThumbnail - Failed to read from stream");
      return E_INVALIDARG;
    }
    dump(L"GetThumbnail - Read from stream");
  }
  else if (!img.load(path_)) {
    dump(L"GetThumbnail - Fail");
    return E_INVALIDARG;
  }

#if 0
  // XXX Docs say that Windows will do the scaling for us.
  if (image.getWidth() > cx || image.getHeight() > cx) {
    image.rescale(cx, cx, FILTER_CATMULLROM, true);
  }
#endif

  if (img.getImageType() != FIT_BITMAP && img.convertToType(FIT_BITMAP)) {
    dump(L"GetThumbnail - noconv");
    return E_INVALIDARG;
  }

  switch (img.getColorType()) {
  case FIC_RGBALPHA:
  case FIC_RGB:
    break;

  default:
    if (!img.convertTo32Bits()) {
      dump(L"GetThumbnail - noconv32");
      return E_UNEXPECTED;
    }
    break;
  }

  BYTE* bits = nullptr;
  auto info = img.getInfo();
  *phbmp = ::CreateDIBSection(
    nullptr,
    info,
    DIB_RGB_COLORS,
    (void**)&bits,
    nullptr,
    0);
  if (!*phbmp) {
    dump(L"GetThumbnail - nodib");
    return E_OUTOFMEMORY;
  }
  if (!bits) {
    dump(L"GetThumbnail - nodib2");
    return E_OUTOFMEMORY;
  }

  auto dib = FreeImage_GetBits(img);
  if (!dib) {
    DeleteObject(*phbmp);
    *phbmp = nullptr;
    return E_OUTOFMEMORY;
  }

  const auto size = img.getPitch() * img.getHeight();
  memcpy(bits, dib, size);
  dump(L"GetThumbnail - success");
  return S_OK;
}