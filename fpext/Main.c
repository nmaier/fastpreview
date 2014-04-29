#define WIN32_LEAN_AND_MEAN	
#include <windows.h>

extern HRESULT RegisterServers();
extern HRESULT UnregisterServers();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdReason, LPVOID lpvReserved)
{
       return 1;
}
//---------------------------------------------------------------------------
 
HRESULT __declspec(dllexport) DllRegisterServer(void)
{
    return RegisterServers();
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

HRESULT __declspec(dllexport) DllUnregisterServer(void)
{
    return UnregisterServers();
}