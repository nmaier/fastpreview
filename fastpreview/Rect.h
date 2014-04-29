#pragma once

#include <windows.h>

class Rect : public RECT
{
public:
  Rect()
  {
    left = right = top = bottom = 0;
  }

  Rect(const RECT& r)
  {
    memcpy(this, &r, sizeof(RECT));
  }

  Rect(const LONG w, const LONG h)
  {
    left = top = 0;
    right = w;
    bottom = h;
  }

  Rect(
    const LONG l, const LONG t, const LONG rw, const LONG bh,
    const bool isWH = false)
  {
    left = l;
    top = t;
    right = rw;
    bottom = bh;
    if (isWH) {
      right += left;
      bottom += top;
    }
  }

  LONG width() const
  {
    return right - left;
  }

  LONG height() const
  {
    return bottom - top;
  }

  Rect& centerIn(const RECT& r)
  {
    const LONG w = width(), h = height();
    left = r.left + ((r.right - r.left) - width()) / 2;
    top = r.top + ((r.bottom - r.top) - height()) / 2;
    right = left + w;
    bottom = top + h;
    return *this;
  }
};

class WorkArea : public Rect
{
public:
  WorkArea(HWND hwnd_)
  {
    HMONITOR mon = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX moninfo;
    moninfo.cbSize = sizeof(moninfo);
    GetMonitorInfo(mon, &moninfo);
    memcpy(this, &moninfo.rcWork, sizeof(RECT));
  }
};