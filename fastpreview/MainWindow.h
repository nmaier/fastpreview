#pragma once

#include <windows.h>
#include <string>
#include <shellapi.h>
#include <memory>

#include "Registry.h"
#include "Exception.h"
#include "WatcherThread.h"
#include "functions.h"
#include "FileAttr.h"
#include "FreeImagePlus.h"

#define MESSAGEHANDLER(handler) \
  LRESULT __fastcall handler(UINT msg, WPARAM wparam, LPARAM lparam)

class MainWindow
{
  friend LRESULT CALLBACK MainWindowProc(HWND, UINT, WPARAM, LPARAM);

private:
  std::wstring file_;
  FileAttr fileAttr_;
  const HINSTANCE hinst_;

  UINT clientWidth_, clientHeight_;

  HWND hwnd_, hstatus_;
  HDC hmem_;
  HMENU hctx_;
  bool statusShowing_;

  FreeImage::WinImage img_;

  std::unique_ptr<WatcherThread> watcher_;
  float aspect_, bestAspect_, newAspect_;
  bool best_, wheeling_;

  POINT sp_;
  POINT dcDims_;

  bool inTransformation_;
  BOOL keyCtrl_, keyShift_;
  BOOL mbtnDown_;

  POINT mbtnPos_;

  Registry reg_;

public:
  MainWindow(HINSTANCE aInstance, const std::wstring &aFile);
  ~MainWindow(void);

  void Show();

protected:
  LRESULT WindowProc(HWND hwnd_, UINT msg, WPARAM wparam, LPARAM lparam);
  LRESULT Perform(UINT Msg, WPARAM wParam, LPARAM lParam) const
  {
    return SendMessage(hwnd_, Msg, wParam, lParam);
  }

private:
  void PreAdjustWindow();
  void CenterWindow() const;
  void LoadFile();
  void FreeFile();
  void CreateDC();
  void DoDC();
  void ProcessPaint() const;

  void DeleteMe();
  void Switch();

  void SetStatus(const std::wstring &aText = _T(""));

  void SetTitle();

  UINT Width() const
  {
    return img_.isValid() ?
      (UINT)((float)img_.getWidth() * (best_ ? bestAspect_ : aspect_)) :
      clientWidth_;
  }

  UINT Height() const
  {
    return img_.isValid() ?
      (UINT)((float)img_.getHeight() * (best_ ? bestAspect_ : aspect_)) :
      clientHeight_;
  }

  FREE_IMAGE_FILTER GetResampleMethod() const;
  void SetResampleMethod(FREE_IMAGE_FILTER aMethod);

  void Transform(FREE_IMAGE_JPEG_OPERATION aTrans);

  void BrowseNew();
  static HICON GetIcon(const std::wstring& File);

protected:
  MESSAGEHANDLER(OnClose);
  MESSAGEHANDLER(OnShowWindow);
  MESSAGEHANDLER(OnPaint);
  MESSAGEHANDLER(OnKey);
  MESSAGEHANDLER(OnChar);
  MESSAGEHANDLER(OnMButton);
  MESSAGEHANDLER(OnLButton);
  MESSAGEHANDLER(OnMouseMove);
  MESSAGEHANDLER(OnScroll);
  MESSAGEHANDLER(OnWheel);
  MESSAGEHANDLER(OnContextMenu);
  MESSAGEHANDLER(OnCommand);
  MESSAGEHANDLER(OnSysCommand);
  MESSAGEHANDLER(OnWatch);
  MESSAGEHANDLER(OnTimer);
  MESSAGEHANDLER(OnMoving);
  MESSAGEHANDLER(OnSize);
};
