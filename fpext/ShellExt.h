#ifndef SHELLEXT_H
#define SHELLEXT_H
#pragma once

#include <windows.h>
#include <shlobj.h>
#include <comdef.h>
#include <Registry.h>
#include <FreeImagePlus.h>
#include <thumbcache.h>

#include <memory>

// {FD669B05-A3B7-425c-99C8-64CD0A270CE1}
static const GUID IID_ShellExt = {
  0xfd669b05, 0xa3b7, 0x425c,
  { 0x99, 0xc8, 0x64, 0xcd, 0xa, 0x27, 0xc, 0xe1 }
};
static const wchar_t cShellExt[] = L"{FD669B05-A3B7-425c-99C8-64CD0A270CE1}";

#define IFP_DESC L"FastPreview ShellExt Server"
#define IFP_VERSION 3
#define IFP_PROGID L"FastPreview.ShellExt"

extern HINSTANCE gModThis;

class ShellExt
  : public IShellExtInit, public IContextMenu3, public IShellPropSheetExt,
  public IThumbnailProvider, public IInitializeWithItem,
  public IInitializeWithStream, public IInitializeWithFile
{
  friend DWORD WINAPI OptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

public:
  ShellExt();
  ~ShellExt();

  virtual IFACEMETHODIMP QueryInterface(REFIID iid, void ** ppvObject);
  virtual IFACEMETHODIMP_(ULONG) AddRef(void);
  virtual IFACEMETHODIMP_(ULONG) Release(void);

  // IShellExtInit
  virtual IFACEMETHODIMP Initialize(
    LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

  // IInitializeWithItem
  virtual IFACEMETHODIMP Initialize(IShellItem *psi, DWORD grfMode);

  // InitializeWithStream
  virtual IFACEMETHODIMP Initialize(IStream *pstream, DWORD grfMode);

  // IntialzeWithFile
  virtual IFACEMETHODIMP Initialize(LPCWSTR pszFilePath, DWORD grfMode);

  // IContextMenu
  virtual IFACEMETHODIMP GetCommandString(
    UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);

  virtual IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);

  virtual IFACEMETHODIMP QueryContextMenu(
    HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);

  // IContextMenu2
  virtual IFACEMETHODIMP HandleMenuMsg(
    UINT uMsg, WPARAM wParam, LPARAM lParam);

  // IContextMenu3
  virtual IFACEMETHODIMP HandleMenuMsg2(
    UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

  // IShellPropSheetExt
  virtual IFACEMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
  virtual IFACEMETHODIMP ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM);

  // IThumbnailProvider
  virtual IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);

  void initializeFromRegistry();

  static void ShellExt::createView(HWND hwnd, const std::wstring& path);

private:
  struct releaser
  {
    void operator ()(IUnknown* r)
    {
      if (r) {
        r->Release();
      }
    }
  };

  static UINT grefcnt;

  ULONG refcnt_;

  std::wstring path_;
  std::unique_ptr<IStream, releaser> stream_;
  FreeImage::WinImage image_;
  FreeImage::StaticInformation info_;
  Registry reg_;
  WTS_ALPHATYPE alpha_;

  uint32_t width_, height_;
  uint32_t maxSize_;
  bool showThumb_;

  void ShellExt::showOptions(HWND hWnd);
};

extern DWORD WINAPI OptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#endif // SHELLEXT_H
