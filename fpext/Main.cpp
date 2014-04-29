#include <windows.h>
#include <FreeImage.h>

EXTERN_C HRESULT RegisterServers();
EXTERN_C HRESULT UnregisterServers();

HINSTANCE gModThis = 0;

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdReason, LPVOID lpvReserved)
{
  if (fwdReason == DLL_PROCESS_ATTACH) {
    FreeImage_Initialise(TRUE);
    gModThis = hinstDLL;
  }
  else if (fwdReason == DLL_PROCESS_DETACH) {
    FreeImage_DeInitialise();
  }
  return 1;
}
//---------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
  return RegisterServers();
}

STDAPI DllUnregisterServer(void)
{
  return UnregisterServers();
}