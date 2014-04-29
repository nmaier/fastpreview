// ==========================================================
// MultiPage class implementation
//
// Design and implementation by
// - Hervé Drolon (drolon@infonie.fr)
//
// This file is part of FreeImage 3
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// ==========================================================

#include "FreeImagePlus.h"

using namespace FreeImage;

MultiPage::MultiPage(bool keep_cache_in_memory) : _mpage(NULL), _bMemoryCache(keep_cache_in_memory) {
}

MultiPage::~MultiPage() {
	if(_mpage) {
		// close the stream
		close(0);
	}
}

bool MultiPage::isValid() const {
	return (NULL != _mpage) ? true : false;
}

bool MultiPage::open(const std::string& lpszPathName, bool create_new, bool read_only, int flags) {
	// try to guess the file format from the filename
	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(lpszPathName.c_str());

	// open the stream
	_mpage = FreeImage_OpenMultiBitmap(fif, lpszPathName.c_str(), create_new, read_only, _bMemoryCache, flags);

	return (NULL != _mpage ) ? true : false;
}

bool MultiPage::open(MemoryIO& memIO, int flags) {
	// try to guess the file format from the memory handle
	FREE_IMAGE_FORMAT fif = memIO.getFileType();

	// open the stream
	_mpage = memIO.loadMultiPage(fif, flags);

	return (NULL != _mpage ) ? true : false;
}

bool MultiPage::open(FreeImageIO *io, fi_handle handle, int flags) {
	// try to guess the file format from the handle
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromHandle(io, handle, 0);

	// open the stream
	_mpage = FreeImage_OpenMultiBitmapFromHandle(fif, io, handle, flags);

	return (NULL != _mpage ) ? true : false;
}

bool MultiPage::close(int flags) {
	bool bSuccess = false;
	if(_mpage) {
		// close the stream
		bSuccess = FreeImage_CloseMultiBitmap(_mpage, flags);
		_mpage = NULL;
	}

	return bSuccess;
}

bool MultiPage::saveToHandle(FREE_IMAGE_FORMAT fif, FreeImageIO *io, fi_handle handle, int flags) const {
	bool bSuccess = false;
	if(_mpage) {
		bSuccess = FreeImage_SaveMultiBitmapToHandle(fif, _mpage, io, handle, flags);
	}

	return bSuccess;
}

bool MultiPage::saveToMemory(FREE_IMAGE_FORMAT fif, MemoryIO& memIO, int flags) const {
	bool bSuccess = false;
	if(_mpage) {
		bSuccess = memIO.saveMultiPage(fif, _mpage, flags);
	}

	return bSuccess;
}

int MultiPage::getPageCount() const {
	return _mpage ? FreeImage_GetPageCount(_mpage) : 0;
}

void MultiPage::appendPage(Image& image) {
	if(_mpage) {
		FreeImage_AppendPage(_mpage, image);
	}
}

void MultiPage::insertPage(int page, Image& image) {
	if(_mpage) {
		FreeImage_InsertPage(_mpage, page, image);
	}
}

void MultiPage::deletePage(int page) {
	if(_mpage) {
		FreeImage_DeletePage(_mpage, page);
	}
}

bool MultiPage::movePage(int target, int source) {
	return _mpage ? FreeImage_MovePage(_mpage, target, source) : false;
}

FIBITMAP* MultiPage::lockPage(int page) {
	return _mpage ? FreeImage_LockPage(_mpage, page) : NULL;
}

void MultiPage::unlockPage(Image& image, bool changed) {
	if(_mpage) {
		FreeImage_UnlockPage(_mpage, image, changed);
		// clear the image so that it becomes invalid.
		// this is possible because of the friend declaration
		image._dib = NULL;
		image._bHasChanged = false;
	}
}

bool MultiPage::getLockedPageNumbers(int *pages, int *count) const {
	return _mpage ? FreeImage_GetLockedPageNumbers(_mpage, pages, count) : false;
}

