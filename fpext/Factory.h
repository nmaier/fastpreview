#pragma once

#include <windows.h>
#include <shlobj.h>
#include <comdef.h>

class Factory : public IClassFactory
{
public:
  // IUnknown
  virtual IFACEMETHODIMP QueryInterface(const IID& iid, void** ppv);
  virtual IFACEMETHODIMP_(ULONG) AddRef();
  virtual IFACEMETHODIMP_(ULONG) Release();

  // IClassFactory
  virtual IFACEMETHODIMP CreateInstance(
    IUnknown* pUnkOuter, REFIID iid, void** ppv);
  virtual IFACEMETHODIMP LockServer(BOOL bLock);

  Factory() : refcnt_(1)
  {}

  virtual ~Factory()
  {}

private:
  long refcnt_;
};