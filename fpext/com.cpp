#include <objbase.h>

#include "Factory.h"
#include "ShellExt.h"
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
  if (rclsid != IID_ShellExt) {
    return CLASS_E_CLASSNOTAVAILABLE;
  }

  auto factory = new(std::nothrow) Factory();
  if (!factory) {
    return E_OUTOFMEMORY;
  }

  auto hr = factory->QueryInterface(riid, ppv);
  factory->Release();
  return hr;
}