#include <objbase.h>

#include "Factory.h"
#include "FPShellExt.h"
#include "ComServers.h"
#include "stringtools.h"

static ULONG Processes = 0;

STDAPI DllCanUnloadNow(void)
{
  return S_OK;
}

_Check_return_
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
  if (rclsid != IID_IFPShellExt) {
    return CLASS_E_CLASSNOTAVAILABLE;
  }

  IFPFactory* factory = new(std::nothrow) IFPFactory();
  if (!factory) {
    return E_OUTOFMEMORY;
  }

  HRESULT hr = factory->QueryInterface(riid, ppv);
  factory->Release();
  return hr;
}