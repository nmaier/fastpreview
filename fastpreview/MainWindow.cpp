#include "MainWindow.h"

#include <strsafe.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <math.h>
#include <sstream>

#include "resource.h"
#include "console.h"
#include "Messages.h"
#include "Objects.h"
#include "Rect.h"
#include "procpriority.h"

/* timers */
#define IDT_RELOAD   1
#define IDT_AACCEPT  3

/* status */
#define IDC_STATUS 10

namespace {
  using stringtools::loadResourceString;

  const std::wstring s_switch = loadResourceString(IDS_SWITCH);
  const std::wstring s_switch_free = loadResourceString(IDS_SWITCH_FREE);
  const std::wstring s_switch_best = loadResourceString(IDS_SWITCH_BEST);
  const std::wstring s_opening = loadResourceString(IDS_OPENING);
  const std::wstring s_quitting = loadResourceString(IDS_QUITTING);
  const std::wstring s_deleting = loadResourceString(IDS_DELETING);
  const std::wstring s_explorermenu = loadResourceString(IDS_EXPLORERMENU);
  const std::wstring s_loading = loadResourceString(IDS_LOADING);
  const std::wstring s_resizing = loadResourceString(IDS_RESIZING);
  const std::wstring s_err_load = loadResourceString(IDS_ERR_LOAD);
  const std::wstring s_err_unsupported = loadResourceString(IDS_ERR_UNSUPPORTED);

  class MenuMethod
  {
  private:
    typedef std::map<FREE_IMAGE_FILTER, DWORD> MenuMap;
    const MenuMap menuMap_;

    static MenuMap makeMenuMap()
    {
      MenuMap menuMap;
      menuMap[FILTER_BOX] = ID_RESAMPLEMETHOD_BOX;
      menuMap[FILTER_BILINEAR] = ID_RESAMPLEMETHOD_BILINEAR;
      menuMap[FILTER_BICUBIC] = ID_RESAMPLEMETHOD_BICUBIC;
      menuMap[FILTER_BSPLINE] = ID_RESAMPLEMETHOD_BSPLINE;
      menuMap[FILTER_CATMULLROM] = ID_RESAMPLEMETHOD_CATMULL;
      menuMap[FILTER_LANCZOS3] = ID_RESAMPLEMETHOD_LANCZOS3;
      return menuMap;
    }

  public:
    MenuMethod() : menuMap_(makeMenuMap())
    {}
    void operator ()(HMENU hSub, FREE_IMAGE_FILTER m) const
    {
      MENUITEMINFO mi;
      mi.cbSize = sizeof(MENUITEMINFO);
      mi.fMask = MIIM_STATE;
      for (const auto& i : menuMap_) {
        mi.fState = i.first == m ? MFS_CHECKED : MFS_UNCHECKED;
        SetMenuItemInfo(
          hSub,
          i.second,
          FALSE,
          &mi
          );
      }
    }

    bool valid(DWORD f) const
    {
      return menuMap_.find((FREE_IMAGE_FILTER)f) != menuMap_.end();
    }

  };

  class __declspec(novtable) AutoToggle
  {
  private:
    bool &ref_;

  public:
    explicit AutoToggle(bool& ref, bool initial) : ref_(ref)
    {
      ref_ = initial;
    }
    ~AutoToggle()
    {
      ref_ = !ref_;
    }
  };
}


static LRESULT CALLBACK MainWindowProc(
  HWND hwnd_, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  MainWindow *Owner = reinterpret_cast<MainWindow*>(
    GetProp(hwnd_, _T("ownerWndProc")));

  switch (uMsg) {
  case WM_CREATE:
    SetProp(
      hwnd_,
      _T("ownerWndProc"),
      (HANDLE)reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams
      );
    break;
  }

  return Owner->WindowProc(hwnd_, uMsg, wParam, lParam);
}

MainWindow::MainWindow(HINSTANCE aInstance, const std::wstring &aFile)
  : hinst_(aInstance),
  file_(aFile),
  fileAttr_(aFile),
  clientWidth_(300), clientHeight_(300),
  aspect_(1.0f),
  newAspect_(1.0f),
  best_(true),
  wheeling_(false),
  hmem_(nullptr),
  inTransformation_(false),
  keyCtrl_(FALSE),
  keyShift_(FALSE),
  mbtnDown_(FALSE),
  reg_(HKEY_CURRENT_USER, L"Software\\MaierSoft\\FastPreview")
{
  InitCommonControls();

  reg_.create();

  // setup window class and register
  WNDCLASS wc;
  wc.style = CS_OWNDC | CS_DBLCLKS;
  wc.lpfnWndProc = MainWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hinst_;
  wc.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APP));
  //wc.hIcon = GetIcon(file);
  wc.hCursor = nullptr; //LoadCursor(nullptr, MAKEINTRESOURCE(IDC_ARROW));
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = _T("FPWND");
  RegisterClass(&wc);

  size_t pos = file_.rfind('\\') + 1;

  // Create the window!
  auto title =
    stringtools::formatResourceString(IDS_MAINTITLE, file_.substr(pos).c_str());
  hwnd_ = CreateWindowEx(
    WS_EX_APPWINDOW,
    _T("FPWND"),
    title.c_str(),
    WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    nullptr,
    nullptr,
    hinst_,
    (LPVOID)this
    );
  if (!hwnd_) {
    throw WindowsException();
  }

  mbtnPos_.x = mbtnPos_.y = 0;

  hctx_ = LoadMenu(
    hinst_,
    MAKEINTRESOURCE(IDR_MAINCTX)
    );
  hstatus_ = CreateWindowEx(
    WS_EX_STATICEDGE,
    STATUSCLASSNAME,
    nullptr,
    WS_CHILD | WS_BORDER,
    0, 0, 0, 0,
    hwnd_,
    nullptr, //(HMENU)IDC_STATUS,
    hinst_,
    nullptr
    );
  if (!hstatus_ || hstatus_ == INVALID_HANDLE_VALUE) {
    throw WindowsException();
  }

  SendMessage(hstatus_, SB_SIMPLE, TRUE, 0);
  CreateDC();
}

MainWindow::~MainWindow(void)
{
  KillTimer(hwnd_, IDT_RELOAD);

  DestroyWindow(hwnd_);
  UnregisterClass(_T("FPWND"), hinst_);
  DestroyMenu(hctx_);
  DeleteDC(hmem_);
}

LRESULT MainWindow::WindowProc(
  HWND hwnd_, UINT msg, WPARAM wparam, LPARAM lparam)
{
#define ONHANDLER(message, handler) \
  case message: return handler(msg, wparam, lparam)

  switch (msg) {
  case WM_CREATE:	{
    SetTimer(
      hwnd_,
      IDT_RELOAD,
      2000,
      0
      );
    SetCursor(
      LoadCursor(
      nullptr,
      IDC_ARROW
      )
      );

    HMENU hSys = GetSystemMenu(hwnd_, FALSE);
    InsertMenu(hSys, SC_CLOSE, MF_STRING | MF_BYCOMMAND, WM_USER, _T("&About"));
    InsertMenu(hSys, SC_CLOSE, MF_SEPARATOR | MF_BYCOMMAND, 0, nullptr);

    return 0;
  }

    ONHANDLER(WM_CLOSE, OnClose);

    ONHANDLER(WM_SHOWWINDOW, OnShowWindow);

    ONHANDLER(WM_PAINT, OnPaint);

    ONHANDLER(WM_KEYUP, OnKey);
    ONHANDLER(WM_KEYDOWN, OnKey);

    ONHANDLER(WM_CHAR, OnChar);

    ONHANDLER(WM_MBUTTONDOWN, OnMButton);
    ONHANDLER(WM_MBUTTONUP, OnMButton);

    ONHANDLER(WM_LBUTTONDOWN, OnLButton);
    ONHANDLER(WM_LBUTTONUP, OnLButton);
    ONHANDLER(WM_NCMOUSEMOVE, OnLButton);
    ONHANDLER(WM_LBUTTONDBLCLK, OnLButton);

    ONHANDLER(WM_MOUSEMOVE, OnMouseMove);

    ONHANDLER(WM_VSCROLL, OnScroll);
    ONHANDLER(WM_HSCROLL, OnScroll);

    ONHANDLER(WM_MOUSEWHEEL, OnWheel);

    ONHANDLER(WM_CONTEXTMENU, OnContextMenu);

    ONHANDLER(WM_COMMAND, OnCommand);
    ONHANDLER(WM_SYSCOMMAND, OnSysCommand);

    ONHANDLER(WM_TIMER, OnTimer);

    ONHANDLER(WM_MOVING, OnMoving);

    ONHANDLER(WM_SIZE, OnSize);
    ONHANDLER(WM_SIZING, OnSize);

    ONHANDLER(WM_WATCH, OnWatch);

  default:
    return ::DefWindowProc(hwnd_, msg, wparam, lparam);
  }
#undef ONHANDLER
}

#define HANDLERIMPL(handler) \
  LRESULT __fastcall MainWindow::handler(UINT msg, WPARAM wparam, LPARAM lparam)
#define HANDLERPASS \
  return ::DefWindowProc(hwnd_, msg, wparam, lparam)

HANDLERIMPL(OnClose)
{
  PostQuitMessage(0);
  return 0;
}

HANDLERIMPL(OnShowWindow)
{
  CenterWindow();
  HANDLERPASS;
}

HANDLERIMPL(OnPaint)
{
  ConWrite(L"Paint");
  PAINTSTRUCT ps;
  BeginPaint(hwnd_, &ps);
  ConWrite(itos(ps.rcPaint.left));
  ConWrite(itos(ps.rcPaint.top));
  ConWrite(itos(ps.rcPaint.right));
  ConWrite(itos(ps.rcPaint.bottom));
  ConWrite(itos(sp_.x));
  ConWrite(itos(sp_.y));
  if (ps.rcPaint.right - ps.rcPaint.left && ps.rcPaint.bottom - ps.rcPaint.top) {
    BitBlt(
      ps.hdc,
      ps.rcPaint.left,
      ps.rcPaint.top,
      ps.rcPaint.right - ps.rcPaint.left,
      ps.rcPaint.bottom - ps.rcPaint.top,
      hmem_,
      sp_.x + ps.rcPaint.left,
      sp_.y + ps.rcPaint.top,
      SRCCOPY
      );
  }

  EndPaint(hwnd_, &ps);

  SendMessage(hstatus_, msg, wparam, lparam);
  return 0;
}

HANDLERIMPL(OnKey)
{
  if (msg == WM_KEYUP) {
    switch (wparam) {
    case VK_SHIFT:
      keyShift_ = FALSE;
      break;

    case VK_CONTROL:
      keyCtrl_ = FALSE;
      break;

    default:
      HANDLERPASS;
    }
    return 0;
  }

  switch (wparam) {
  case VK_SHIFT:
    keyShift_ = TRUE;
    break;

  case VK_CONTROL:
    keyCtrl_ = TRUE;
    break;
  }

  if (keyShift_ && keyCtrl_) {
    HANDLERPASS;
  }
  else if (keyShift_) {
    switch (wparam) {
    case VK_UP:
    case VK_DOWN:
      Transform(FIJPEG_OP_FLIP_V);
      break;

    case VK_LEFT:
    case VK_RIGHT:
      Transform(FIJPEG_OP_FLIP_H);
      break;
    default:
      HANDLERPASS;
    }
    return 0;
  }

  else if (keyCtrl_) {
    switch (wparam) {
    case VK_UP:
    case VK_DOWN:
      Transform(FIJPEG_OP_ROTATE_180);
      break;
    case VK_LEFT:
      Transform(FIJPEG_OP_ROTATE_270);
      break;
    case VK_RIGHT:
      Transform(FIJPEG_OP_ROTATE_90);
      break;

    default:
      HANDLERPASS;
    }
    return 0;
  }

  switch (wparam) {
  case VK_ESCAPE:
    PostQuitMessage(0);
    break;

  case VK_DELETE:
    DeleteMe();
    break;

  case VK_UP:
  case VK_LEFT:
    PostMessage(
      hwnd_,
      wparam == VK_UP ? WM_VSCROLL : WM_HSCROLL,
      MAKEWPARAM(SB_LINEUP, 0),
      0
      );
    break;

  case VK_DOWN:
  case VK_RIGHT:
    PostMessage(
      hwnd_,
      wparam == VK_DOWN ? WM_VSCROLL : WM_HSCROLL,
      MAKEWPARAM(SB_LINEDOWN, 0),
      0
      );
    break;

  case VK_F5:
    LoadFile();
    break;

  case VK_RETURN:
    Switch();
    break;

  default:
    HANDLERPASS;
  }

  return 0;
}

HANDLERIMPL(OnChar)
{
  switch (toupper((int)wparam)) {
  case 'A':
    Switch();
    break;
  case '-':
  case '+':
    if (!best_) {
      newAspect_ = (wchar_t)wparam == '+'
        ? min(5.0f, newAspect_ + 0.1f)
        : max(0.1f, newAspect_ - 0.1f);
      SetTitle();
      KillTimer(hwnd_, IDT_AACCEPT);
      SetTimer(hwnd_, IDT_AACCEPT, 500, nullptr);
    }
    break;

  case '#':
  case '0':
    if (!best_) {
      aspect_ = 1.0f;
      DoDC();
    }
    break;

  case 'R':
    LoadFile();
    break;

  default:
    HANDLERPASS;
  }

  return 0;
}

HANDLERIMPL(OnMButton)
{
  switch (msg) {
  case WM_MBUTTONDOWN:
    if (img_.isValid()) {
      const FreeImage::Information& info = img_.getOriginalInformation();
      wchar_t mp[20];
      StringCchPrintf(mp, 20, L"%.2f",
        (double)info.getWidth() * info.getHeight() / 1000.0 / 1000.0);
      SetStatus(stringtools::formatResourceString(
        IDS_INFOS,
        info.getWidth(),
        info.getHeight(),
        stringtools::convert(info.getFormat().getName()).c_str(),
        mp
        ).c_str());
    }
    return FALSE;

  case WM_MBUTTONUP:
    SetStatus();
    return FALSE;
  }
  HANDLERPASS;
}

HANDLERIMPL(OnLButton)
{
  switch (msg) {
  case WM_LBUTTONDOWN: {
    ConWrite(L"BtnDown");
    mbtnDown_ = TRUE;

    mbtnPos_.x = GET_X_LPARAM(lparam);
    mbtnPos_.y = GET_Y_LPARAM(lparam);
    SetCursor(
      LoadCursor(
      nullptr,
      IDC_SIZEALL
      )
      );
    return 0;
  }

  case WM_NCMOUSEMOVE:
  case WM_LBUTTONUP: {
    ConWrite(L"BtnUp");
    mbtnDown_ = FALSE;
    SetCursor(
      LoadCursor(
      nullptr,
      IDC_ARROW
      )
      );
    if (msg == WM_NCMOUSEMOVE) {
      break;
    }
    return 0;
  }

  case WM_LBUTTONDBLCLK: {
    SHELLEXECUTEINFO sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = hwnd_;
    sei.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_INVOKEIDLIST;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpVerb = L"properties";
    sei.lpFile = file_.c_str();
    ShellExecuteEx(&sei);
    return 0;
  }

  } // switch

  HANDLERPASS;
}

HANDLERIMPL(OnMouseMove)
{
  if (mbtnDown_) {
    ConWrite(L"Move");

    Perform(
      WM_HSCROLL,
      MAKEWPARAM(9, (mbtnPos_.x - GET_X_LPARAM(lparam)) * 1.4f),
      0
      );
    Perform(
      WM_VSCROLL,
      MAKEWPARAM(9, (mbtnPos_.y - GET_Y_LPARAM(lparam)) * 1.4f),
      0
      );
    mbtnPos_.x = GET_X_LPARAM(lparam);
    mbtnPos_.y = GET_Y_LPARAM(lparam);
  }
  HANDLERPASS;
}

HANDLERIMPL(OnScroll)
{
  const bool vscroll = (msg == WM_VSCROLL);
  int SB = vscroll ? SB_VERT : SB_HORZ;
  ConWrite(itos(SB));

  SCROLLINFO si;
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_PAGE | SIF_RANGE;
  if (!GetScrollInfo(hwnd_, SB, &si)) {
    return 0;
  }

  if ((signed)si.nPage > si.nMax) {
    return 0;
  }

  LONG v = vscroll ? sp_.y : sp_.x;

  switch (LOWORD(wparam)) {
  case SB_LINEDOWN:
    v += 10;
    break;

  case SB_LINEUP:
    v -= 10;
    break;

  case SB_PAGEDOWN:
    v += 100;
    break;

  case SB_PAGEUP:
    v -= 100;
    break;

  case SB_THUMBPOSITION:
  case SB_THUMBTRACK:
    v = (short)HIWORD(wparam);
    break;

  case 9:
    v += GET_WHEEL_DELTA_WPARAM(wparam);
    break;
  }

  ConWrite(L"una:" + itos(v));
  v = max(si.nMin, v);
  v = min(si.nMax - (signed)si.nPage, v);
  ConWrite(L"adj:" + itos(v));
  SetScrollPos(hwnd_, SB, v, TRUE);
  InvalidateRect(hwnd_, nullptr, FALSE);
  vscroll ? sp_.y = v : sp_.x = v;

  return 0;
}

HANDLERIMPL(OnWheel)
{
  if (wheeling_) {
    ConWrite(L"Wheeled!");
    short points = GET_WHEEL_DELTA_WPARAM(wparam) * -10 / WHEEL_DELTA;
    PostMessage(hwnd_, WM_VSCROLL, MAKEWPARAM(9, points), 0);
  }
  return 0;
}

HANDLERIMPL(OnContextMenu)
{
  MENUITEMINFO mi;
  mi.cbSize = sizeof(MENUITEMINFO);

  const std::wstring mt = stringtools::formatResourceString(
    IDS_SWITCH, (best_ ? s_switch_free : s_switch_best).c_str());
  mi.fMask = MIIM_STRING | MIIM_STATE;
  mi.dwTypeData = const_cast<wchar_t*>(mt.c_str());
  mi.cch = static_cast<UINT>(mt.length());
  mi.fState = img_.isValid() ? MFS_ENABLED : MFS_DISABLED;

  SetMenuItemInfo(
    GetSubMenu(hctx_, 0),
    ID_MCTX_SWITCH,
    FALSE,
    &mi
    );

  mi.fMask = MIIM_STATE;
  mi.fState = img_.isValid() &&
    img_.getOriginalInformation().getFormat() == FIF_JPEG ? MFS_ENABLED : MFS_DISABLED;

  HMENU hSub = GetSubMenu(hctx_, 0);
  unsigned subCount = GetMenuItemCount(hSub);
  SetMenuItemInfo(
    hSub,
    subCount - 4,
    TRUE,
    &mi
    );

  MenuMethod menuMethod;
  menuMethod(hSub, GetResampleMethod());

  TrackPopupMenu(
    hSub,
    TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
    GET_X_LPARAM(lparam),
    GET_Y_LPARAM(lparam),
    0,
    hwnd_,
    nullptr
    );

  HANDLERPASS;
}

HANDLERIMPL(OnCommand)
{
  switch (LOWORD(wparam)) {
  case ID_MCTX_OPEN: {
    SetStatus(s_opening.c_str());
    SHELLEXECUTEINFO shi;
    ZeroMemory(&shi, sizeof(shi));
    shi.cbSize = sizeof(shi);
    shi.fMask = SEE_MASK_NOASYNC | SEE_MASK_DOENVSUBST | SEE_MASK_HMONITOR |
      SEE_MASK_NOZONECHECKS;
    shi.lpFile = file_.c_str();
    shi.hMonitor = ::MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    shi.hwnd = hwnd_;
    shi.nShow = SW_SHOWDEFAULT;
    ShellExecuteEx(&shi);
    SetStatus();
    break;
  }

  case ID_MCTX_OPENANOTHER:
    BrowseNew();
    break;

  case ID_MCTX_SHOWPROPERTIES:
    PostMessage(hwnd_, WM_LBUTTONDBLCLK, 0, 0);
    break;

  case ID_MCTX_EXIT:
    SetStatus(s_quitting.c_str());
    PostQuitMessage(0);
    break;

  case ID_MCTX_DELETEFILE:
    SetStatus(s_deleting.c_str());
    DeleteMe();
    SetStatus();
    break;

  case ID_MCTX_EXPLORERMENU:
    SetStatus(s_explorermenu.c_str());
    try {
      ExplorerMenu(hwnd_, file_);
    }
    catch (Exception
#ifdef _DEBUG
      &E
#endif
      ) {
      ConWrite(E.GetString());
    }
    SetStatus();
    break;

  case ID_MCTX_SWITCH:
    Switch();
    break;

  case ID_RESAMPLEMETHOD_BOX:
    SetResampleMethod(FILTER_BOX);
    break;

  case ID_RESAMPLEMETHOD_BILINEAR:
    SetResampleMethod(FILTER_BILINEAR);
    break;

  case ID_RESAMPLEMETHOD_BICUBIC:
    SetResampleMethod(FILTER_BICUBIC);
    break;

  case ID_RESAMPLEMETHOD_BSPLINE:
    SetResampleMethod(FILTER_BSPLINE);
    break;

  case ID_RESAMPLEMETHOD_CATMULL:
    SetResampleMethod(FILTER_CATMULLROM);
    break;

  case ID_RESAMPLEMETHOD_LANCZOS3:
    SetResampleMethod(FILTER_LANCZOS3);
    break;

  case ID_LOSSLESSTRANSFORM_LEFT:
    Transform(FIJPEG_OP_ROTATE_270);
    break;

  case ID_LOSSLESSTRANSFORM_RIGHT:
    Transform(FIJPEG_OP_ROTATE_90);
    break;

  case ID_LOSSLESSTRANSFORM_181:
    Transform(FIJPEG_OP_ROTATE_180);
    break;

  case ID_LOSSLESSTRANSFORM_VERTICALFLIP:
    Transform(FIJPEG_OP_FLIP_V);
    break;

  case ID_LOSSLESSTRANSFORM_HORIZONTALFLIP:
    Transform(FIJPEG_OP_FLIP_H);
    break;

  }

  return 0;
}

HANDLERIMPL(OnSysCommand)
{
  if (wparam == WM_USER) {
    MessageBox(
      hwnd_,
      _T("FastPreview 4.0\n\n© 2006-2014 by Nils Maier\n\nSee the License.* files for more licensing information."),
      _T("About"),
      MB_ICONINFORMATION
      );
    return 0;
  }

  HANDLERPASS;
}

HANDLERIMPL(OnWatch)
{
  ConWrite(_T("Watch"));
  if (inTransformation_) {
    HANDLERPASS;
  }

  switch (wparam) {
  case FILE_ACTION_REMOVED:
    ConWrite(L"RECV D");
    PostQuitMessage(0);
    break;
  default:
    try {
      FileAttr(file_).getSize();
    }
    catch (Exception) {
      ConWrite(L"RECV D2");
      PostQuitMessage(0);
    }
    ConWrite(L"RECV REL");
    LoadFile();
    break;
  }
  return 0;
}

HANDLERIMPL(OnTimer)
{
  switch (wparam) {
  case IDT_RELOAD:
  {
    ConWrite(_T("LOAD"));
    if (inTransformation_) {
      break;
    }
    try {
      if (FileAttr(file_) != fileAttr_) {
        LoadFile();
      }
    }
    catch (Exception) {
      PostQuitMessage(0);
    }
  }
    break;
  case IDT_AACCEPT:
    KillTimer(hwnd_, IDT_AACCEPT);
    if (newAspect_ != aspect_) {
      aspect_ = newAspect_;
      DoDC();
    }
    break;
  }

  return 0;
}
HANDLERIMPL(OnMoving)
{
  WorkArea area(hwnd_);
  Rect *rc = reinterpret_cast<Rect*>(lparam);
  auto dx = (rc->left < area.left) ?
    (area.left - rc->left) :
    ((rc->right > area.right) ? (area.right - rc->right) : 0);
  auto dy = (rc->top < area.top) ?
    (area.top - rc->top) :
    ((rc->bottom > area.bottom) ? (area.bottom - rc->bottom) : 0);
  OffsetRect(rc, dx, dy);

  return TRUE;
}

HANDLERIMPL(OnSize)
{
  if (msg == WM_SIZE) {
    SendMessage(hstatus_, msg, wparam, lparam);
    HANDLERPASS;
  }

  RECT r;
  GetWindowRect(hwnd_, &r);
  CopyRect(reinterpret_cast<LPRECT>(lparam), &r);
  return TRUE;
}

void MainWindow::Show()
{
  LoadFile();
  ShowWindow(hwnd_, SW_SHOWNORMAL);

  try {
    watcher_.reset(new WatcherThread(file_, hwnd_));
  }
  catch (Exception &E) {
    E.show(hwnd_);
  }
}

void MainWindow::PreAdjustWindow()
{
  WINDOWINFO wi;
  ZeroMemory(&wi, sizeof(WINDOWINFO));
  GetWindowInfo(
    hwnd_,
    &wi
    );
  Rect wr(wi.rcWindow), cr(wi.rcClient);

  WorkArea area(hwnd_);

  unsigned
    maxX = area.width() - (wr.width() - cr.width()),
    maxY = area.height() - (wr.height() - cr.height());

  if (best_ && img_.isValid()) {
    bestAspect_ = 1.0f;
    if ((unsigned)img_.getWidth() > maxX) {
      bestAspect_ = (float)maxX / (float)img_.getWidth();
    }
    if (Height() > maxY) {
      bestAspect_ = (float)maxY / (float)img_.getHeight();
    }
  }

  const bool hs = Width() > maxX, vs = Height() > maxY;

  SCROLLINFO si = {
    sizeof(si),
    SIF_PAGE | SIF_RANGE,
    0,
    0,
    0,
    0,
    0
  };
  if (hs) {
    si.nPage = maxX;
    si.nMax = Width() + (vs ? GetSystemMetrics(SM_CXVSCROLL) : 0);
    ConWrite(L"hs-p: " + itos(si.nPage) + L" hs-m: " + itos(si.nMax));
  }
  SetScrollInfo(
    hwnd_,
    SB_HORZ,
    &si,
    TRUE
    );
  if (vs) {
    si.nPage = maxY;
    si.nMax = Height() + (hs ? GetSystemMetrics(SM_CYHSCROLL) : 0);
  }

  SetScrollInfo(
    hwnd_,
    SB_VERT,
    &si,
    TRUE
    );

  wheeling_ = hs || vs;

  if (vs) {
    ShowScrollBar(hwnd_, SB_VERT, TRUE);
  }
  if (hs) {
    ShowScrollBar(hwnd_, SB_HORZ, TRUE);
  }


  clientHeight_ = min(maxY, Height());
  clientWidth_ = min(maxX, Width());
  if (hs) {
    clientHeight_ = min(clientHeight_ + GetSystemMetrics(SM_CYHSCROLL), maxY);
  }
  if (vs) {
    clientWidth_ = min(clientWidth_ + GetSystemMetrics(SM_CXVSCROLL), maxX);
  }

  Rect rw(clientWidth_, clientHeight_);
  AdjustWindowRectEx(
    &rw,
    wi.dwStyle,
    FALSE,
    wi.dwExStyle
    );

  MoveWindow(
    hwnd_,
    wr.left,
    wr.top,
    rw.width(),
    rw.height(),
    TRUE
    );
}

void MainWindow::LoadFile()
{
  if (inTransformation_) {
    return;
  }
  SetStatus(s_loading.c_str());
  FreeFile();

  fileAttr_ = FileAttr(file_);

  img_.clear();
  bool load;
  {
    Priority prio(HIGH_PRIORITY_CLASS);
    load = img_.load(file_.c_str());
  }

  if (!load) {
    sp_.x = sp_.y = 0;
    clientHeight_ = clientWidth_ = 400;

    ShowScrollBar(hwnd_, SB_BOTH, FALSE);
    PreAdjustWindow();

    CreateDC();

    if (hmem_ == INVALID_HANDLE_VALUE) {
      throw WindowsException();
    }
    SelectObject(
      hmem_,
      GetStockObject(DEFAULT_GUI_FONT)
      );

    RECT  rc = {0, 0, (LONG)Width(), (LONG)Height()};
    FillRect(
      hmem_,
      &rc,
      (HBRUSH)(COLOR_BTNFACE + 1)
      );
    if (!DrawText(
      hmem_,
      s_err_load.c_str(),
      -1,
      &rc,
      DT_SINGLELINE | DT_CENTER | DT_VCENTER
      )) {
      throw WindowsException();
    }

    CenterWindow();
    SetTitle();
    InvalidateRect(hwnd_, nullptr, FALSE);
  }
  else {
    if (img_.getFormat() == FIF_BMP && img_.getBitsPerPixel() == 32) {
      img_.convertTo24Bits();
    }
    DoDC();
  }
  SetStatus();
}

void MainWindow::DoDC()
{
  ConWrite(_T("DoDC"));
  ShowScrollBar(hwnd_, SB_BOTH, FALSE);
  PreAdjustWindow();
  SetTitle();

  CreateDC();

  const UINT left = (dcDims_.x - Width()) / 2;
  const UINT top = (dcDims_.y - Height()) / 2;

  float fasp = fabs(1.f - (best_ ? bestAspect_ : aspect_));
  if (fasp < 1e-4) {
    img_.draw(hmem_, Rect(left, top, left + Width(), top + Height()));
    ConWrite(itos(Width()) + _T("-") + itos(clientWidth_));
  }
  else {
    SetStatus(s_resizing.c_str());
    const UINT w = Width();
    const UINT h = Height();
    try {
      FreeImage::WinImage r(img_);
      r.rescale(w, h, GetResampleMethod());
      r.draw(hmem_, Rect(left, top, left + w, top + h));
    }
    catch (std::exception& ex) {
      SetStatus(stringtools::convert(ex.what()));
    }

    SetStatus();
  }

  SHFILEINFO shfi;
  shfi.hIcon = 0;
  shfi.iIcon = 0;

  std::wstring e = file_.substr(0, file_.rfind('.') + 1);
  e.append(stringtools::convert(img_.getOriginalInformation().getFormat().getFirstExtension()));

  UINT iconType = SHGFI_LARGEICON;
  if (Width() < 160 || Height() < 160) {
    iconType = SHGFI_SMALLICON;
  }

  HIMAGELIST hImgList = (HIMAGELIST)SHGetFileInfo(
    e.c_str(),
    FILE_ATTRIBUTE_NORMAL,
    &shfi,
    sizeof(SHFILEINFO),
    SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | iconType
    );

  if (hImgList != nullptr) {
    if (iconType == SHGFI_SMALLICON) {
      ImageList_Draw(hImgList, shfi.iIcon, hmem_, dcDims_.x - 5 - GetSystemMetrics(SM_CXSMICON), 5, ILD_TRANSPARENT);
      //DrawIcon(hmem_, Width() - 5 - GetSystemMetrics(SM_CXICON), 5, shfi.hIcon);
    }
    else {
      ImageList_Draw(hImgList, shfi.iIcon, hmem_, Width() - 10 - GetSystemMetrics(SM_CXICON), 10, ILD_TRANSPARENT);
    }
    if (shfi.hIcon != 0) {
      DestroyIcon(shfi.hIcon);
    }
  }
  sp_.x = sp_.y = 0;
  CenterWindow();
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::FreeFile()
{
  img_.clear();
}

void MainWindow::CreateDC()
{
  if (hmem_ != nullptr) {
    DeleteDC(hmem_);
    hmem_ = nullptr;
  }
  hmem_ = CreateCompatibleDC(nullptr);

  Rect cr;
  GetClientRect(hwnd_, &cr);
  const UINT width = max(Width(), (UINT)cr.width());
  const UINT height = max(Height(), (UINT)cr.height());


  DeleteObject(SelectObject(hmem_, CreateCompatibleBitmap(GetDC(hwnd_), width, height)));
  SetBkMode(
    hmem_,
    TRANSPARENT
    );
  const Rect r(0, 0, width, height);
  FillRect(hmem_, &r, (HBRUSH)(COLOR_WINDOW + 1));
  dcDims_.x = width;
  dcDims_.y = height;
}

void MainWindow::ProcessPaint() const
{
  for (MSG msg; PeekMessage(&msg, hwnd_, 0, 0, PM_REMOVE | PM_QS_PAINT);) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void MainWindow::DeleteMe()
{
  SetStatus(s_deleting.c_str());
  auto pFile = std::make_unique <wchar_t[]>(file_.length() + 2);
  wcscpy_s(
    pFile.get(),
    file_.length() + 2,
    file_.c_str()
    );

  SHFILEOPSTRUCT op = {
    hwnd_,
    FO_DELETE,
    pFile.get(),
    nullptr,
    FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_FILESONLY,
    FALSE,
    nullptr,
    nullptr
  };

  Thread::Locker l(watcher_.get());
  SHFileOperation(
    &op
    );

  SetStatus();
}

void MainWindow::Switch()
{
  if (img_.isValid()) {
    best_ = !best_;
    DoDC();
  }
}

void MainWindow::SetTitle()
{
  std::wstringstream ss;
  ss << stringtools::formatResourceString(
    IDS_MAINTITLE, file_.substr(file_.rfind('\\') + 1).c_str());
  ss << L" - ";
  if (img_.isValid()) {
    const FreeImage::Information& info = img_.getOriginalInformation();
    wchar_t mp[20];
    StringCchPrintf(mp, 20, L"%.2f",
      (double)info.getWidth() * info.getHeight() / 1000.0 / 1000.0);
    ss << stringtools::formatResourceString(
      IDS_INFOS,
      info.getWidth(),
      info.getHeight(),
      stringtools::convert(info.getFormat().getName()).c_str(),
      mp
      );
    ss << stringtools::formatResourceString(
      IDS_SCALE, (int)round(100.0 * (best_ ? bestAspect_ : newAspect_)));
  }
  else {
    ss << s_err_unsupported;
  }
  Perform(
    WM_SETTEXT,
    0,
    (LPARAM)ss.str().c_str()
    );
}

void MainWindow::SetStatus(const std::wstring &aText)
{
  if (aText.empty()) {
    ShowWindow(hstatus_, SW_HIDE);
    statusShowing_ = false;
  }
  else {
    SendMessage(
      hstatus_, SB_SETTEXT, (WPARAM)SB_SIMPLEID, (LPARAM)aText.c_str());
    if (!statusShowing_) {
      ShowWindow(hstatus_, SW_SHOW);
      statusShowing_ = true;
    }
  }
  RECT r;
  GetClientRect(hstatus_, &r);
  InvalidateRect(hwnd_, &r, FALSE);
  ProcessPaint();
}

void MainWindow::CenterWindow() const
{
  Rect rw;
  GetWindowRect(
    hwnd_,
    &rw
    );
  WorkArea wa(hwnd_);
  rw.centerIn(wa);
  MoveWindow(
    hwnd_,
    rw.left,
    rw.top,
    rw.width(),
    rw.height(),
    TRUE
    );
}

FREE_IMAGE_FILTER MainWindow::GetResampleMethod() const
{
  MenuMethod menuMethod;
  FREE_IMAGE_FILTER rv = FILTER_LANCZOS3;
  uint32_t f = 0;
  if (reg_.get(L"appFIResampleMethod", f)) {
    if (menuMethod.valid(f)) {
      rv = (FREE_IMAGE_FILTER)f;
    }
  }
  return rv;
}

void MainWindow::SetResampleMethod(FREE_IMAGE_FILTER m)
{
  reg_.set(L"appFIResampleMethod", (uint32_t)m);
  DoDC();
}

void MainWindow::Transform(FREE_IMAGE_JPEG_OPERATION aTrans)
{
  if (!img_.isValid() || img_.getOriginalInformation().getFormat() != FIF_JPEG) {
    throw Exception("Not a JPEG!");
  }

  try {
    AutoToggle a(inTransformation_, true);
    Thread::Suspender wts(watcher_.get());

    if (!FreeImage_JPEGTransform(
      stringtools::WStringToString(file_).c_str(),
      stringtools::WStringToString(file_).c_str(),
      aTrans
      )) {
      std::wstringstream ss;
      ss << L"Cannot transform " << file_;
      throw Exception(ss.str());
    }
    LoadFile();
  }
  catch (Exception &ex) {
    ex.show(hwnd_, L"Error transforming file.");
  }
}

void MainWindow::BrowseNew()
{
  if (!openDlg->Execute(hwnd_, file_)) {
    return;
  }
  watcher_.reset();
  file_ = openDlg->getFileName();
  watcher_.reset(new WatcherThread(file_, hwnd_));

  LoadFile();
}

HICON MainWindow::GetIcon(const std::wstring& File)
{
  SHFILEINFO shfi;
  ZeroMemory(&shfi, sizeof(SHFILEINFO));
  SHGetFileInfo(
    File.c_str(),
    0,
    &shfi,
    sizeof(SHFILEINFO),
    SHGFI_ICON | SHGFI_SMALLICON
    );
  return shfi.hIcon ?
    shfi.hIcon :
    LoadIcon((HINSTANCE)GetCurrentProcess(), MAKEINTRESOURCE(IDI_APP));
}
