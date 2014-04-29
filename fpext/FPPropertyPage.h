#ifndef FPPROPERTYPAGE_H
#define FPPROPERTYPAGE_H

#include <windows.h>
#include <string>
#include <map>
#include <FreeImagePlus.h>

#include "stringtools.h"
#include "resource.h"

class IFPShellExt;

class FPPropertyPage {
private:
	static const std::wstring title;
	static const std::wstring col_type;
	static const std::wstring col_value;

	std::wstring file_;
	FreeImage::WinImage img_;
	HPROPSHEETPAGE handle_;
	HWND hwnd_, hlist_;
  IFPShellExt *ext_;
	
	static INT_PTR CALLBACK proc(
    HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	static UINT CALLBACK callbackProc(HWND, UINT uMsg, LPPROPSHEETPAGE lparam);
	
	INT_PTR loop(UINT msg, WPARAM wparam, LPARAM lparam);
	void handleCommand(WPARAM wparam, LPARAM lparam);
	void init();
	void drawImg(LPDRAWITEMSTRUCT dis);

public:
	FPPropertyPage(IFPShellExt* ext, const std::wstring& file, UINT& ref);
	virtual ~FPPropertyPage();

	HPROPSHEETPAGE getHandle() const {
		return handle_;
	}
};

#endif