#include "ComServers.h"

#include <memory>
#include <tchar.h>
#include <shlobj.h>

#include <sstream>
#include <functional>
#include <algorithm>

#include "FreeImagePlus.h"
#include "stringtools.h"
#include "Registry.h"

namespace {
  static const wchar_t* blacklisted[] = {
    L"bmp", L"gif", L"jpeg", L"jpe", L"jpg", L"png", L"wbmp", L"ico"
  };
  inline bool isExtBlacklisted(const std::wstring& ext)
  {
    for (const auto& e : blacklisted) {
      if (ext.compare(e) == 0 || ext.compare(1, std::wstring::npos, e) == 0) {
        return true;
      }
    }
    return false;
  }
}

HMODULE COMServers::GetModuleHandle()
{
  HMODULE rv = nullptr;
  GetModuleHandleEx(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    (LPCTSTR)COMServers::GetModuleHandle,
    &rv
    );
  return rv;
}

const std::wstring COMServers::ModulePath()
{
  auto module = std::make_unique<wchar_t[]>(MAX_PATH);
  std::wstring rv;

  if (::GetModuleFileName(
    GetModuleHandle(),
    module.get(),
    MAX_PATH
    )) {
    rv.assign(module.get());
  }
  return rv;
}

const std::wstring COMServers::ModuleDirectory()
{
  auto p = std::make_unique < wchar_t[]>(MAX_PATH);
  wchar_t *o = p.get();
  wcscpy_s(
    o,
    MAX_PATH,
    ModulePath().c_str()
    );
  while (*++o);
  while (--o >= p.get() && *o != '\\');
  *(o + 1) = 0;

  wstring rv(p.get());
  return rv;
}

bool COMServers::Register(
  const wstring& name, const wstring& description, unsigned char version, const wstring& CLSID)
{
  dump(L"register");
#if defined(_DEBUG)
#	define ENSURE(x,y) if (!(x)) { MessageBox(nullptr, std::wstring(wstring(_T("FAIL: ")) + std::wstring(y) + _T("\n")).c_str(), _T("regerr"), 0); Unregister(name, version, CLSID); return false; } else { dump(L"OK: %s", std::wstring(y).c_str()); }
#else
#	define ENSURE(x,y) if (!(x)) { dump(L"FAIL: %s %s", std::wstring(y).c_str(), stringtools::convert(#x).c_str()); Unregister(name, version, CLSID); return false; }
#endif

  wchar_t num[32];
  _itow_s(version, num, 32, 10);
  wstring progId(name);
  progId.append(_T("."));
  progId.append(num);

  wstring clsIdPath(_T("CLSID\\"));
  clsIdPath.append(CLSID);

  wstring module = ModulePath();

  ENSURE(module.size() != 0, _T("modname ") + module);

  wstring prog(COMServers::ModuleDirectory());
  prog.append(_T("fastpreview.exe"));

  /* Dependent */
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, progId, L"", description), L"dprogid");
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, progId + L"\\CLSID", L"", CLSID), L"dprogid/clsid");

  /* Independent */
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, name, L"", description), L"iprogid");
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, name + L"\\CurVer", L"", progId), L"iprogid/curver");

  /* CLSID */
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, clsIdPath, L"", description), _T("clsid"));
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, clsIdPath + L"\\InprocServer32", L"", module), _T("clsid/inproc"));
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, clsIdPath + L"\\InprocServer32", L"ThreadingModel", L"Apartment"), _T("clsid/inproc"));
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, clsIdPath + L"\\ProgID", L"", progId), _T("clsid/progid"));
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, clsIdPath + L"\\VersionIndependentProgID", L"", name), _T("clsid/viprogid"));

  /* Integration */
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, L"*\\ShellEx\\ContextMenuHandlers\\FastPreview", L"", CLSID), L"*/ctx");
  ENSURE(Registry::set(HKEY_CLASSES_ROOT, L"*\\ShellEx\\PropertySheetHandlers\\FastPreview", L"", CLSID), L"*/ctx");

  const auto count = FreeImage_GetFIFCount();
  for (auto i = 0; i < count; ++i) {
    FreeImage::Format fmt((FREE_IMAGE_FORMAT)i);
    if (!fmt.isValid()) {
      continue;
    }

    auto mainext = stringtools::convert(fmt.getName());
    auto name = std::wstring(L"FastPreview.") + mainext;
    auto desc = stringtools::convert(fmt.getDescription());

    ENSURE(Registry::set(HKEY_CLASSES_ROOT, name, L"", desc), L"type");
    ENSURE(Registry::set(HKEY_CLASSES_ROOT, name + L"\\shell\\open\\command", L"", L"\"" + prog + L"\" \"%1\""), L"type shell");

    std::wstring friendly, icon;
    auto maintype = mainext + L"file";
    std::transform(maintype.begin(), maintype.end(), maintype.begin(), ::towlower);

    dump(L"Main type: %s\n", maintype.c_str());
    if (Registry::get(HKEY_CLASSES_ROOT, maintype, L"FriendlyTypeName", friendly) && !friendly.empty()) {
      dump(L"Got friendly on primary (%s): %s\n", maintype.c_str(), friendly.c_str());
      ENSURE(Registry::set(HKEY_CLASSES_ROOT, name, L"FriendlyTypeName", friendly, REG_EXPAND_SZ), L"copy friendly");
    }
    if (Registry::get(HKEY_CLASSES_ROOT, maintype + L"\\DefaultIcon", L"", icon) && !icon.empty()) {
      dump(L"Got icon on primary (%s): %s\n", maintype.c_str(), icon.c_str());
      ENSURE(Registry::set(HKEY_CLASSES_ROOT, name + L"\\DefaultIcon", L"", icon, REG_EXPAND_SZ), L"copy icon");
    }

    std::wstringstream exts(stringtools::convert(fmt.getExtensions()));
    std::wstring ext;
    while (std::getline(exts, ext, L',')) {
      std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

      if (friendly.empty()) {
        maintype = ext + L"file";
        std::transform(maintype.begin(), maintype.end(), maintype.begin(), ::towlower);
        if (Registry::get(HKEY_CLASSES_ROOT, maintype, L"FriendlyTypeName", friendly) && !friendly.empty()) {
          dump(L"Got friendly on secondary (%s): %s\n", maintype.c_str(), friendly.c_str());
          ENSURE(Registry::set(HKEY_CLASSES_ROOT, name, L"FriendlyTypeName", friendly, REG_EXPAND_SZ), L"copy friendly");
        }
      }
      if (icon.empty()) {
        maintype = ext + L"file";
        std::transform(maintype.begin(), maintype.end(), maintype.begin(), ::towlower);
        if (Registry::get(HKEY_CLASSES_ROOT, maintype + L"\\DefaultIcon", L"", icon) && !icon.empty()) {
          dump(L"Got icon on secondary (%s): %s\n", maintype.c_str(), icon.c_str());
          ENSURE(Registry::set(HKEY_CLASSES_ROOT, name + L"\\DefaultIcon", L"", icon, REG_EXPAND_SZ), L"copy icon");
        }
      }

      ext = L"." + ext;
      ENSURE(Registry::set(HKEY_CLASSES_ROOT, ext + L"\\OpenWithProgids", name, L""), L"openwith");
      auto installThumber = !isExtBlacklisted(ext);
      if (installThumber) {
        std::wstring thumber;
        installThumber = !Registry::get(HKEY_CLASSES_ROOT, ext + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}", L"", thumber) ||
          thumber.empty();
        if (installThumber) {
          dump(L"Setting thumber on %s", ext.c_str());
          ENSURE(Registry::set(HKEY_CLASSES_ROOT, ext + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}", L"", CLSID), L"Thumber");
        }
      }
    }

    if (friendly.empty() && fmt.getFirstExtension().compare("bmp") == 0) {
      if (Registry::get(HKEY_CLASSES_ROOT, L"PhotoViewer.FileAssoc.Bitmap", L"FriendlyTypeName", friendly) && !friendly.empty()) {
        dump(L"Got friendly from PhotoViewer (%s): %s\n", name.c_str(), friendly.c_str());
        ENSURE(Registry::set(HKEY_CLASSES_ROOT, name, L"FriendlyTypeName", friendly, REG_EXPAND_SZ), L"copy friendly");
      }
    }
    if (icon.empty() && fmt.getFirstExtension().compare("bmp") != 0) {
      if (Registry::get(HKEY_CLASSES_ROOT, L"jpegfile\\DefaultIcon", L"", icon) && !icon.empty()) {
        dump(L"Got icon from jpegfile (%s): %s\n", maintype.c_str(), icon.c_str());
        ENSURE(Registry::set(HKEY_CLASSES_ROOT, name + L"\\DefaultIcon", L"", icon, REG_EXPAND_SZ), L"copy icon");
      }
    }
    if (icon.empty()) {
      if (Registry::get(HKEY_CLASSES_ROOT, L"PhotoViewer.FileAssoc.Bitmap\\DefaultIcon", L"", icon) && !icon.empty()) {
        dump(L"Got icon from PhotoViewer (%s): %s\n", maintype.c_str(), icon.c_str());
        ENSURE(Registry::set(HKEY_CLASSES_ROOT, name + L"\\DefaultIcon", L"", icon, REG_EXPAND_SZ), L"copy icon");
      }
    }
  }

#undef ENSURE

  ::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  dump(L"done");

  return true;
}
bool COMServers::Unregister(
  const wstring& name,
  const unsigned char version,
  const wstring& CLSID
  )
{
  wchar_t num[32];
  _itow_s(version, num, 32, 10);
  wstring progId(name);
  progId.append(_T("."));
  progId.append(num);

  wstring clsIdPath(_T("CLSID\\"));
  clsIdPath.append(CLSID);

#if 0 || defined(_DEBUG)
#	define ENSURE(x,y) if (!(x)) { MessageBox(nullptr, wstring(wstring(_T("FAIL: ")) + wstring(y) + _T("\n")).c_str(), _T("unregerr"), 0); return false; } else { dump(L"OKunreg: %s", std::wstring(y).c_str()); }
#elif defined(THIS_IS_WOW)
# define ENSURE(x,y) (x)
#else
#	define ENSURE(x,y) if (!(x)) { dump(L"FAIL: %s %s", std::wstring(y).c_str(), stringtools::convert(#x).c_str()); return false; }
#endif

  dump(L"Unregister: %s", CLSID.c_str());
  ENSURE(Registry::remove(HKEY_CLASSES_ROOT, L"*\\ShellEx\\ContextMenuHandlers\\FastPreview"), L"*/ctx");
  ENSURE(Registry::remove(HKEY_CLASSES_ROOT, L"*\\ShellEx\\PropertySheetHandlers\\FastPreview"), L"*/ctx");
  ENSURE(Registry::remove(HKEY_CLASSES_ROOT, progId), L"dprogid");
  ENSURE(Registry::remove(HKEY_CLASSES_ROOT, name), L"iprogid");
  ENSURE(Registry::remove(HKEY_CLASSES_ROOT, clsIdPath), L"clsid");

  dump(L"Unregister types: %s", CLSID.c_str());
  const auto count = FreeImage_GetFIFCount();
  for (auto i = 0; i < count; ++i) {
    FreeImage::Format fmt((FREE_IMAGE_FORMAT)i);
    if (!fmt.isValid()) {
      continue;
    }

    auto name = std::wstring(L"FastPreview.") + stringtools::convert(fmt.getName());
    ENSURE(Registry::remove(HKEY_CLASSES_ROOT, name), L"type");

    std::wstringstream exts(stringtools::convert(fmt.getExtensions()));
    std::wstring ext;
    while (std::getline(exts, ext, L',')) {
      std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
      ext = L"." + ext;
      ENSURE(Registry::unset(HKEY_CLASSES_ROOT, ext + L"\\OpenWithProgids", name), L"openwith");
      std::wstring thumber;
      if (Registry::get(HKEY_CLASSES_ROOT, ext + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}", L"", thumber)) {
        dump(L"Thumber for %s: %s(%d), expected: %s(%d)", ext.c_str(), thumber.c_str(), thumber.length(), CLSID.c_str(), CLSID.length());
        if (thumber.compare(CLSID) == 0) {
          dump(L"Removing thumber on %s", ext.c_str());
          ENSURE(Registry::remove(HKEY_CLASSES_ROOT, ext + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}"), L"thumber");
        }
      }
    }
  }

  /* pref */
#ifndef THIS_IS_WOW
  if (Registry::exists(HKEY_CURRENT_USER, L"Software\\MaierSoft\\FastPreview", L"") &&
    MessageBox(nullptr, L"Do you want to remove settings as well?", L"Setting", MB_YESNO | MB_ICONQUESTION
    ) == IDYES) {
    ENSURE(Registry::remove(HKEY_CURRENT_USER, L"Software\\MaierSoft\\FastPreview"), L"prefs");
  }
#endif

#undef ENSURE

  ::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  dump(L"done");

  return true;
}
