#include "functions.h"
#include "Exception.h"
#include "Console.h"

LRESULT WINAPI CtxListenerProc(HWND hwnd_, UINT msg, WPARAM wparam, LPARAM lparam)
{
  IContextMenu2 *ctx = reinterpret_cast<IContextMenu2*>(GetProp(hwnd_, _T("Ctx")));
  switch (msg) {
  case WM_CREATE:
    SetProp(hwnd_, _T("Ctx"), (HANDLE)reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);
    return 0;

  case WM_DRAWITEM:
  case WM_MEASUREITEM:
    if (wparam)
      break;

  case WM_INITMENUPOPUP:
    ctx->HandleMenuMsg(
      msg,
      wparam,
      lparam
      );
    return (msg == WM_INITMENUPOPUP ? 0 : TRUE);
  }

  return ::DefWindowProc(hwnd_, msg, wparam, lparam);
}

void ExplorerMenu(const HWND& hwnd, const std::wstring &aIn)
{
  IShellFolder *deskptr;
  if (FAILED(SHGetDesktopFolder(&deskptr))) {
    throw Exception(_T("Desk Folder failed!"));
  }
  ComPtr<IShellFolder> desk(deskptr);

  LPITEMIDLIST ppidl;
  auto rv = desk->ParseDisplayName(
    hwnd,
    nullptr,
    (LPOLESTR)aIn.substr(0, aIn.rfind('\\')).c_str(),
    nullptr,
    &ppidl, nullptr);
  if (FAILED(rv)) {
    throw Exception(_T("PDNF failed!"));
  }

  IShellFolder *folderptr;
  rv = desk->BindToObject(ppidl, nullptr, IID_IShellFolder, (void**)&folderptr);
  if (FAILED(rv)) {
    throw Exception(_T("BTO failed!"));
  }
  ComPtr<IShellFolder> folder(folderptr);

  ConWrite(aIn.substr(aIn.rfind('\\') + 1));
  rv = folder->ParseDisplayName(
    hwnd,
    nullptr,
    (LPOLESTR)aIn.substr(aIn.rfind('\\') + 1).c_str(),
    nullptr,
    &ppidl,
    nullptr
    );
  if (FAILED(rv)) {
    throw Exception(_T("PDN failed!"));
  }

  IContextMenu2 *ctxptr = nullptr;
  rv = folder->GetUIObjectOf(
    hwnd, 1, (LPCITEMIDLIST*)&ppidl, IID_IContextMenu, 0, (void**)&ctxptr);
  if (FAILED(rv)) {
    throw Exception(_T("GUOO failed!"));
  }
  ComPtr<IContextMenu2> ctx(ctxptr);

  void *p;
  if (
    FAILED(ctx->QueryInterface(IID_IContextMenu3, &p)) &&
    FAILED(ctx->QueryInterface(IID_IContextMenu2, &p))) {
    throw Exception(_T("No interface"));
  }
  ConWrite(L"OK");
  ctx = ComPtr<IContextMenu2>(reinterpret_cast<IContextMenu2*>(p));

  HMENU hMenu = CreatePopupMenu();
  if (FAILED(ctx->QueryContextMenu(hMenu, 0, 1, MAXDWORD, CMF_NORMAL | CMF_EXPLORE | CMF_INCLUDESTATIC))) {
    throw Exception(_T("QCM FAILED"));
  }

  WNDCLASS clsListener = {
    0,
    (WNDPROC)CtxListenerProc,
    0,
    0,
    GetModuleHandle(nullptr),
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    _T("FPCtxMenuClass")
  };
  RegisterClass(&clsListener);

  HWND hListener = CreateWindowEx(
    0,
    clsListener.lpszClassName,
    L"",
    0,
    0, 0, 0, 0,
    HWND_MESSAGE,
    nullptr,
    nullptr,
    ctx.get()
    );
  if (hListener == INVALID_HANDLE_VALUE) {
    throw WindowsException();
  }

  POINT Mouse;
  GetCursorPos(&Mouse);
  UINT Cmd = TrackPopupMenu(
    hMenu,
    TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD,
    Mouse.x,
    Mouse.y,
    0,
    hListener,
    0
    );

  DestroyWindow(hListener);
  CMINVOKECOMMANDINFO ici = {
    sizeof(CMINVOKECOMMANDINFO),
    0,
    hwnd,
    (LPCSTR)MAKEINTRESOURCE(Cmd - 1),
    nullptr,
    nullptr,
    SW_SHOWDEFAULT,
    0,
    nullptr
  };
  ctx->InvokeCommand(&ici);
  DestroyMenu(hMenu);
}