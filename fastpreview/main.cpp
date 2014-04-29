#include <windows.h>

#include <algorithm>
#include <memory>
#include <string>
#include <FreeImagePlus.h>

#include "MainWindow.h"
#include "glue.h"
#include "functions.h"
#include "Objects.h"
#include "FileAttr.h"
#include "resource.h"
#include "stringtools.h"

namespace {
  using stringtools::loadResourceString;

  static const char *defFilters[] = {
    "jpeg", "png", "pcx", "psd", "gif", "bmp", "ico", "webp", nullptr
  };
  static const std::wstring s_err_givefile = loadResourceString(IDS_ERR_GIVEFILE);
  static const std::wstring s_err_init_un = loadResourceString(IDS_ERR_INIT_UN);
  static const std::wstring s_information = loadResourceString(IDS_INFORMATION);
  static const std::wstring s_error = loadResourceString(IDS_ERROR);
}

static std::wstring getFileFromDlg()
{
  if (openDlg->Execute()) {
    Registry::set(HKEY_CURRENT_USER, L"Software\\MaierSoft\\FastPreview", L"appInitialDir", openDlg->getInitialDir());
    return openDlg->getFileName();
  }
  return L"";
}

#ifdef FREEIMAGE_LIB
class FreeImageGlue {
public:
  FreeImageGlue() {
    FreeImage_Initialise(TRUE);
  }
  ~FreeImageGlue() {
    FreeImage_DeInitialise();
  }
};
#endif

int CALLBACK wWinMain(HINSTANCE hinst, HINSTANCE, LPWSTR, int nCmdShow)
{
  {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
  }

  std::wstring file;

  CoGlue coGlue;

#ifdef FREEIMAGE_LIB
  FreeImageGlue fiGlue;
#endif

  std::wstring defFilter;
  OpenDlg::FilterList filters;
  for (const char **e = defFilters; *e; ++e) {
    std::wstring mask;
    FreeImage::Format fmt(*e);
    std::string exts = fmt.getExtensions();
    for (size_t pos = exts.find(','), op = 0uLL;
      op != std::string::npos; op = pos, pos = exts.find(',', pos + 1)) {
      if (!mask.empty()) {
        mask.append(_T(";"));
      }
      mask.append(_T("*."));
      mask.append(stringtools::StringToWString(exts.substr(op, pos)));
    }
    if (!mask.empty()) {
      std::wstring fn(stringtools::StringToWString(fmt.getFirstExtension()));
      std::transform(fn.begin(), fn.end(), fn.begin(), toupper);
      filters.push_back(
        OpenDlg::FilterType(
        fn
        + _T(": ")
        + stringtools::StringToWString(fmt.getDescription())
        ,
        mask
        )
        );
      if (!defFilter.empty()) {
        defFilter.append(_T(";"));
      }
      defFilter.append(mask);
    }
  }
  filters.push_front(OpenDlg::FilterType(_T("Image Files"), defFilter));
  filters.push_back(OpenDlg::FilterType(_T("All Files"), _T("*.*")));

  std::wstring initialAppDir;
  Registry::get(HKEY_CURRENT_USER, L"Software\\MaierSoft\\FastPreview", L"appInitialDir", initialAppDir);
  openDlg = std::make_unique<OpenDlg>(filters, initialAppDir);

  LPWSTR *sArgs;
  int nArgs = 0;
  sArgs = CommandLineToArgvW(
    GetCommandLineW(),
    &nArgs
    );

  if (nArgs != 2) {
    file = getFileFromDlg();
  }
  else {
    file = sArgs[1];
  }
  if (sArgs) {
    GlobalFree(sArgs);
  }

  if (file.empty()) {
    MessageBox(nullptr, s_err_givefile.c_str(), s_information.c_str(), MB_ICONINFORMATION);
    return 0;
  }

  try {
    if (!FileAttr(file).getSize()) {
      throw Exception(L"empty file!");
    }
  }
  catch (Exception) {
    MessageBox(
      nullptr,
      stringtools::formatResourceString(IDS_ERR_FILENOTFOUND, file.c_str()).c_str(),
      s_error.c_str(),
      MB_ICONERROR
      );
    return 0;
  }

  std::unique_ptr<MainWindow> window;
  try {
    window = std::make_unique<MainWindow>(hinst, file.c_str());
    window->Show();
  }
  catch (Exception &ex) {
    MessageBox(
      nullptr,
      stringtools::formatResourceString(IDS_ERR_INIT, ex.GetString().c_str()).c_str(),
      s_error.c_str(),
      MB_ICONERROR
      );
    return 0;
  }
  catch (...) {
    MessageBox(
      nullptr,
      s_err_init_un.c_str(),
      s_error.c_str(),
      MB_ICONERROR
      );
    return 0;

  }
  for (MSG msg; GetMessage(&msg, nullptr, 0, 0) > 0;) {
    try {
      if (msg.message == WM_QUIT) {
        break;
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    catch (Exception &E) {
      E.show(nullptr);
    }
  }

  return 0;
}