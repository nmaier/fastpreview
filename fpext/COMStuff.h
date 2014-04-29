#pragma once
#include <windows.h>

#define DLLEXPORTTYPE extern "C" __declspec(dllexport) HRESULT

DLLEXPORTTYPE DllCanUnloadNow(void);
DLLEXPORTTYPE DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);