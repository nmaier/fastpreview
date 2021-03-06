#include "Factory.h"
#include <windows.h>

#include "ComServers.h"
#include "stringtools.h"
#include "ShellExt.h"

IFACEMETHODIMP Factory::QueryInterface(const IID& iid, LPVOID* ppv)
{
  dump(L"Factory");
  if ((iid != IID_IUnknown) && (iid != IID_IClassFactory)) {
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
  *ppv = static_cast<IClassFactory *>(this);
  reinterpret_cast<IUnknown*>(*ppv)->AddRef();
  return S_OK;
}

IFACEMETHODIMP_(ULONG) Factory::AddRef()
{
  return ::InterlockedIncrement(&refcnt_);
}

IFACEMETHODIMP_(ULONG) Factory::Release()
{
  if (::InterlockedDecrement(&refcnt_) == 0) {
    delete this;
    return 0;
  }
  return refcnt_;
}

IFACEMETHODIMP Factory::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppv)
{
  dump(L"CreateInstance");

  if (pUnkOuter) {
    dump(L"CreateInstance - NoAggr");
    return CLASS_E_NOAGGREGATION;
  }
  ShellExt* inst = new(std::nothrow) ShellExt();

  if (!inst) {
    dump(L"CreateInstance - OOM");
    return E_OUTOFMEMORY;
  }

  // Get the requested interface.
  HRESULT hr = inst->QueryInterface(iid, ppv);

  if (FAILED(hr)) {
    dump(L"CreateInstance - Failed");
    delete inst;
  }
  else {
    dump(L"CreateInstance - OK");
  }
  return hr;
}

IFACEMETHODIMP Factory::LockServer(BOOL bLock)
{
  bLock ? AddRef() : Release();
  return S_OK;
}

extern "C" HRESULT RegisterServers()
{
  return COMServers::Register(IFP_PROGID, IFP_DESC, IFP_VERSION, cShellExt)
    ? S_OK
    : SELFREG_E_CLASS;
}

extern "C" HRESULT UnregisterServers()
{
  return COMServers::Unregister(IFP_PROGID, IFP_VERSION, cShellExt)
    ? S_OK
    : SELFREG_E_CLASS;
}