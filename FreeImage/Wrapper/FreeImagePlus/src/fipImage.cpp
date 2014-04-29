// ==========================================================
// Image class implementation
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

///////////////////////////////////////////////////////////////////   
// Protected functions

bool Image::replace(FIBITMAP *new_dib) {
	if(new_dib == 0) 
		return false;
	if(_dib)
		FreeImage_Unload(_dib);
	_dib = new_dib;
	_bHasChanged = true;
	return true;
}

///////////////////////////////////////////////////////////////////
// Creation & Destruction

Image::Image(FREE_IMAGE_TYPE image_type, unsigned width, unsigned height, unsigned bpp) {
	_dib = 0;
	_bHasChanged = false;
	if(width && height && bpp) {
		setSize(image_type, width, height, bpp);
		_origInfo = StaticInformation(*this);
	}

}

Image::~Image() {
	if(_dib) {
		FreeImage_Unload(_dib);
		_dib = 0;
	}
}

bool Image::setSize(FREE_IMAGE_TYPE image_type, unsigned width, unsigned height, unsigned bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {
	if(_dib) {
		FreeImage_Unload(_dib);
	}
	if((_dib = FreeImage_AllocateT(image_type, width, height, bpp, red_mask, green_mask, blue_mask)) == 0)
		return false;

	if(image_type == FIT_BITMAP) {
		// Create palette if needed
		switch(bpp)	{
			case 1:
			case 4:
			case 8:
				RGBQUAD *pal = FreeImage_GetPalette(_dib);
				for(unsigned i = 0; i < FreeImage_GetColorsUsed(_dib); i++) {
					pal[i].rgbRed = i;
					pal[i].rgbGreen = i;
					pal[i].rgbBlue = i;
				}
				break;
		}
	}

	_origInfo = StaticInformation(*this);
	_bHasChanged = true;

	return true;
}

void Image::clear() {
	if(_dib) {
		FreeImage_Unload(_dib);
		_dib = 0;
	}
	_origInfo = StaticInformation();
	_bHasChanged = true;
}

///////////////////////////////////////////////////////////////////
// Copying

Image::Image(const Image& Image) {
	_dib = 0;
	_format = FIF_UNKNOWN;
	FIBITMAP *clone = FreeImage_Clone((FIBITMAP*)Image._dib);
	replace(clone);
	if (_dib) {
		_origInfo = StaticInformation(*this);
	}
	else {
		_origInfo = StaticInformation();
	}
}

Image& Image::operator=(const Image& Image) {
	if(this != &Image) {
		FIBITMAP *clone = FreeImage_Clone((FIBITMAP*)Image._dib);
		replace(clone);
		if (_dib) {
			_origInfo = StaticInformation(*this);
		}
		else {
			_origInfo = StaticInformation();
		}
	}
	return *this;
}

Image& Image::operator=(FIBITMAP *dib) {
	if(_dib != dib) {
		replace(dib);
		if (_dib) {
			_origInfo = StaticInformation(*this);
		}
		else {
			_origInfo = StaticInformation();
		}
	}
	return *this;
}

bool Image::copySubImage(Image& dst, int left, int top, int right, int bottom) const {
	if(_dib) {
		dst = FreeImage_Copy(_dib, left, top, right, bottom);
		return dst.isValid();
	}
	return false;
}

bool Image::pasteSubImage(Image& src, int left, int top, int alpha) {
	if(_dib) {
		bool bResult = FreeImage_Paste(_dib, src._dib, left, top, alpha);
		_bHasChanged = true;
		return bResult;
	}
	return false;
}

bool Image::crop(int left, int top, int right, int bottom) {
	if(_dib) {
		FIBITMAP *dst = FreeImage_Copy(_dib, left, top, right, bottom);
		return replace(dst);
	}
	return false;
}


///////////////////////////////////////////////////////////////////
// Information functions

FREE_IMAGE_TYPE Image::getImageType() const {
	return FreeImage_GetImageType(_dib);
}

unsigned Image::getWidth() const {
	return FreeImage_GetWidth(_dib); 
}

unsigned Image::getHeight() const {
	return FreeImage_GetHeight(_dib); 
}

unsigned Image::getScanWidth() const {
	return FreeImage_GetPitch(_dib);
}

bool Image::isValid() const {
	return (_dib != 0) ? true:false;
}

BITMAPINFO* Image::getInfo() const {
	return FreeImage_GetInfo(_dib);
}

BITMAPINFOHEADER* Image::getInfoHeader() const {
	return FreeImage_GetInfoHeader(_dib);
}

LONG Image::getImageSize() const {
	if (!isValid()) {
		return 0;
	}
	return FreeImage_GetDIBSize(_dib);
}

unsigned Image::getBitsPerPixel() const {
	if (!isValid()) {
		return 0;
	}
	return FreeImage_GetBPP(_dib);
}

unsigned Image::getLine() const {
	if (!isValid()) {
		return 0;
	}
	return FreeImage_GetLine(_dib);
}

WORD Image::getPitch() const {
	if (!isValid()) {
		return 0;
	}
	return FreeImage_GetPitch(_dib);
}

double Image::getHorizontalResolution() const {
	if (!isValid()) {
		return 0;
	}
	return (FreeImage_GetDotsPerMeterX(_dib) / (double)100); 
}

double Image::getVerticalResolution() const {
	if (!isValid()) {
		return 0;
	}
	return (FreeImage_GetDotsPerMeterY(_dib) / (double)100);
}

void Image::setHorizontalResolution(double value) {
	if (!isValid()) {
		return;
	}
	FreeImage_SetDotsPerMeterX(_dib, (unsigned)(value * 100 + 0.5));
}

void Image::setVerticalResolution(double value) {
	if (!isValid()) {
		return;
	}
	FreeImage_SetDotsPerMeterY(_dib, (unsigned)(value * 100 + 0.5));
}


///////////////////////////////////////////////////////////////////
// Palette operations

RGBQUAD* Image::getPalette() const {
	return FreeImage_GetPalette(_dib);
}

unsigned Image::getPaletteSize() const {
	return FreeImage_GetColorsUsed(_dib) * sizeof(RGBQUAD);
}

unsigned Image::getColorsUsed() const {
	return FreeImage_GetColorsUsed(_dib);
}

FREE_IMAGE_COLOR_TYPE Image::getColorType() const { 
	return FreeImage_GetColorType(_dib);
}

bool Image::isGrayscale() const {
	return ((FreeImage_GetBPP(_dib) == 8) && (FreeImage_GetColorType(_dib) != FIC_PALETTE)); 
}

///////////////////////////////////////////////////////////////////
// Thumbnail access
#if 0
bool Image::getThumbnail(Image& image) const {
	image = FreeImage_Clone( FreeImage_GetThumbnail(_dib) );
	return image.isValid();
}

bool Image::setThumbnail(const Image& image) {
	return FreeImage_SetThumbnail(_dib, (FIBITMAP*)image._dib);
}

bool Image::hasThumbnail() const {
	return (FreeImage_GetThumbnail(_dib) != 0);
}

bool Image::clearThumbnail() {
	return FreeImage_SetThumbnail(_dib, 0);
}
#endif

///////////////////////////////////////////////////////////////////
// Pixel access

BYTE* Image::accessPixels() const {
	return FreeImage_GetBits(_dib); 
}

BYTE* Image::getScanLine(unsigned scanline) const {
	if(scanline < FreeImage_GetHeight(_dib)) {
		return FreeImage_GetScanLine(_dib, scanline);
	}
	return 0;
}

bool Image::getPixelIndex(unsigned x, unsigned y, BYTE *value) const {
	return FreeImage_GetPixelIndex(_dib, x, y, value);
}

bool Image::getPixelColor(unsigned x, unsigned y, RGBQUAD *value) const {
	return FreeImage_GetPixelColor(_dib, x, y, value);
}

bool Image::setPixelIndex(unsigned x, unsigned y, BYTE *value) {
	_bHasChanged = true;
	return FreeImage_SetPixelIndex(_dib, x, y, value);
}

bool Image::setPixelColor(unsigned x, unsigned y, RGBQUAD *value) {
	_bHasChanged = true;
	return FreeImage_SetPixelColor(_dib, x, y, value);
}

///////////////////////////////////////////////////////////////////
// File type identification
#if 0
FREE_IMAGE_FORMAT Image::identifyFIF(const std::string& lpszPathName) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and get its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(lpszPathName.c_str(), 0);
	if(fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName.c_str());
	}

	return fif;
}
#endif

FREE_IMAGE_FORMAT Image::identifyFIF(const std::wstring& lpszPathName) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and get its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileTypeU(lpszPathName.c_str(), 0);
	if(fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilenameU(lpszPathName.c_str());
	}

	return fif;
}
#if 0
FREE_IMAGE_FORMAT Image::identifyFIFFromHandle(FreeImageIO *io, fi_handle handle) {
	if(io && handle) {
		// check the file signature and get its format
		return FreeImage_GetFileTypeFromHandle(io, handle, 16);
	}
	return FIF_UNKNOWN;
}

FREE_IMAGE_FORMAT Image::identifyFIFFromMemory(FIMEMORY *hmem) {
	if(hmem != 0) {
		return FreeImage_GetFileTypeFromMemory(hmem, 0);
	}
	return FIF_UNKNOWN;
}
#endif


///////////////////////////////////////////////////////////////////
// Loading & Saving
#if 0
bool Image::load(const std::string& file, int flag) {
	// check the file signature and get its format
	// (the second argument is currently not used by FreeImage)
	Format loadingFormat = FileFormat(file);
	if (!loadingFormat.isValid()) {
		// no signature ?
		// try to guess the file format from the file extension
		loadingFormat = FileTypeFormat(file);
	}
	// check that the plugin has reading capabilities ...
	if (!loadingFormat.isValid() || !loadingFormat.supportsReading()) {
		return false;
	}

	// Load the file
	FIBITMAP *loadingBitmap = FreeImage_Load(loadingFormat, file.c_str(), flag);
	if(loadingBitmap == 0) {
		return false;
	}
	// Free the previous dib
	if(_dib) {
		FreeImage_Unload(_dib);
	}
	_bHasChanged = true;
	_dib = loadingBitmap;
	_format = loadingFormat;
	
	_origInfo = StaticInformation(*this);

	return true;
}
#endif

bool Image::load(const std::wstring& file, int flag) {
	// check the file signature and get its format
	// (the second argument is currently not used by FreeImage)
	Format loadingFormat = FileFormat(file);
	if (!loadingFormat.isValid()) {
		// no signature ?
		// try to guess the file format from the file extension
		loadingFormat = FileTypeFormat(file);
	}
	// check that the plugin has reading capabilities ...
	if (!loadingFormat.isValid() || !loadingFormat.supportsReading()) {
		return false;
	}
	std::string lfd = loadingFormat.getDescription(), lfk = loadingFormat.getName();
	const char *clfd = lfd.c_str(), *clfk = lfk.c_str();
	OutputDebugStringA(clfd);
	OutputDebugStringA(" - ");
	OutputDebugStringA(clfk);
	OutputDebugStringW(file.c_str());
	// Load the file
	FIBITMAP *loadingBitmap = FreeImage_LoadU(loadingFormat, file.c_str(), flag);
	if(loadingBitmap == 0) {
		return false;
	}
	// Free the previous dib
	if(_dib) {
		FreeImage_Unload(_dib);
	}
	_bHasChanged = true;
	_dib = loadingBitmap;
	_format = loadingFormat;

	_origInfo = StaticInformation(*this);

	return true;
}

#if 0
bool Image::loadFromHandle(FreeImageIO *io, fi_handle handle, int flag) {
	// check the file signature and get its format
	Format format(io, handle);
	if(!format.isValid() || !format.supportsReading()) {
		return false;
	}

	// Load the file
	FIBITMAP *bitmap = FreeImage_LoadFromHandle(format, io, handle, flag);
	if(bitmap == 0) {
		return false;
	}
	if (_dib) {
		FreeImage_Unload(_dib);			
	}
	_bHasChanged = true;
	_format = format;
	_dib = bitmap;

	_origInfo = StaticInformation(*this);

	return true;
}

bool Image::loadFromMemory(MemoryIO& memIO, int flags) {

	Format format(memIO);
	if (!format.isValid() || !format.supportsReading()) {
		return false;
	}

	FIBITMAP *bitmap = memIO.load(format, flags);
	if (bitmap == 0) {
		return false;
	}

	_bHasChanged = true;
	_format = format;
	_dib = bitmap;

	_origInfo = StaticInformation(*this);

	return true;
}
#endif

#if 0
bool Image::save(const std::string& file, int flag) const {

	Format format = FileTypeFormat(file);
	if (!format.isValid() || !format.supportsWriting()) {
		return false;
	}

	FREE_IMAGE_TYPE type = FreeImage_GetImageType(_dib);
	if (type == FIT_BITMAP && !format.supportsBPP(FreeImage_GetBPP(_dib))) {
		return false;
	}
	if (type != FIT_BITMAP && !format.supportsExportType(type)) {
		return false;
	}

	return FreeImage_Save(format, _dib, file.c_str(), flag) == true;
}

bool Image::save(const std::wstring& file, int flag) const {

	Format format = FileTypeFormat(file);
	if (!format.isValid() || !format.supportsWriting()) {
		return false;
	}

	FREE_IMAGE_TYPE type = FreeImage_GetImageType(_dib);
	if (type == FIT_BITMAP && !format.supportsBPP(FreeImage_GetBPP(_dib))) {
		return false;
	}
	if (type != FIT_BITMAP && !format.supportsExportType(type)) {
		return false;
	}

	return FreeImage_SaveU(format, _dib, file.c_str(), flag) == true;
}

bool Image::saveToHandle(FREE_IMAGE_FORMAT fif, FreeImageIO *io, fi_handle handle, int flag) const {
	Format format(fif);
	if (!format.isValid() || !format.supportsWriting()) {
		return false;
	}

	FREE_IMAGE_TYPE type = FreeImage_GetImageType(_dib);
	if (type == FIT_BITMAP && !format.supportsBPP(FreeImage_GetBPP(_dib))) {
		return false;
	}
	if (type != FIT_BITMAP && !format.supportsExportType(type)) {
		return false;
	}

	return FreeImage_SaveToHandle(format, _dib, io, handle, flag) == true;
}

bool Image::saveToMemory(FREE_IMAGE_FORMAT fif, MemoryIO& memIO, int flag) const {
	Format format(fif);
	if (!format.isValid() || !format.supportsWriting()) {
		return false;
	}

	FREE_IMAGE_TYPE type = FreeImage_GetImageType(_dib);
	if (type == FIT_BITMAP && !format.supportsBPP(FreeImage_GetBPP(_dib))) {
		return false;
	}
	if (type != FIT_BITMAP && !format.supportsExportType(type)) {
		return false;
	}

	return memIO.save(format, _dib, flag);
}
#endif
///////////////////////////////////////////////////////////////////   
// Conversion routines

bool Image::convertToType(FREE_IMAGE_TYPE image_type, bool scale_linear) {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToType(_dib, image_type, scale_linear);
		return replace(dib);
	}
	return false;
}
#if 0
bool Image::threshold(BYTE T) {
	if(_dib) {
		FIBITMAP *dib1 = FreeImage_Threshold(_dib, T);
		return replace(dib1);
	}
	return false;
}
#endif

bool Image::convertTo4Bits() {
	if(_dib) {
		FIBITMAP *dib4 = FreeImage_ConvertTo4Bits(_dib);
		return replace(dib4);
	}
	return false;
}

bool Image::convertTo8Bits() {
	if(_dib) {
		FIBITMAP *dib8 = FreeImage_ConvertTo8Bits(_dib);
		return replace(dib8);
	}
	return false;
}

bool Image::convertTo16Bits555() {
	if(_dib) {
		FIBITMAP *dib16_555 = FreeImage_ConvertTo16Bits555(_dib);
		return replace(dib16_555);
	}
	return false;
}

bool Image::convertTo16Bits565() {
	if(_dib) {
		FIBITMAP *dib16_565 = FreeImage_ConvertTo16Bits565(_dib);
		return replace(dib16_565);
	}
	return false;
}

bool Image::convertTo24Bits() {
	if(_dib) {
		FIBITMAP *dibRGB = FreeImage_ConvertTo24Bits(_dib);
		return replace(dibRGB);
	}
	return false;
}

bool Image::convertTo32Bits() {
	if(_dib) {
		FIBITMAP *dib32 = FreeImage_ConvertTo32Bits(_dib);
		return replace(dib32);
	}
	return false;
}

bool Image::convertToGrayscale() {
	if(_dib) {
		FIBITMAP *dib8 = FreeImage_ConvertToGreyscale(_dib);
		return replace(dib8);
	}
	return false;
}

bool Image::colorQuantize(FREE_IMAGE_QUANTIZE algorithm) {
	if(_dib) {
		FIBITMAP *dib8 = FreeImage_ColorQuantize(_dib, algorithm);
		return replace(dib8);
	}
	return false;
}

bool Image::dither(FREE_IMAGE_DITHER algorithm) {
	if(_dib) {
		FIBITMAP *dib = FreeImage_Dither(_dib, algorithm);
		return replace(dib);
	}
	return false;
}

bool Image::convertToFloat() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToFloat(_dib);
		return replace(dib);
	}
	return false;
}

bool Image::convertToRGBF() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToRGBF(_dib);
		return replace(dib);
	}
	return false;
}

bool Image::convertToUINT16() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToUINT16(_dib);
		return replace(dib);
	}
	return false;
}

bool Image::convertToRGB16() {
	if(_dib) {
		FIBITMAP *dib = FreeImage_ConvertToRGB16(_dib);
		return replace(dib);
	}
	return false;
}

bool Image::toneMapping(FREE_IMAGE_TMO tmo, double first_param, double second_param, double third_param, double fourth_param) {
	if(_dib) {
		FIBITMAP *dst = 0;
		// Apply a tone mapping algorithm and convert to 24-bit 
		switch(tmo) {
			case FITMO_REINHARD05:
				dst = FreeImage_TmoReinhard05Ex(_dib, first_param, second_param, third_param, fourth_param);
				break;
			default:
				dst = FreeImage_ToneMapping(_dib, tmo, first_param, second_param);
				break;
		}

		return replace(dst);
	}
	return false;
}

///////////////////////////////////////////////////////////////////   
// Transparency support: background colour and alpha channel
#if 0
bool Image::isTransparent() const {
	return FreeImage_IsTransparent(_dib);
}

unsigned Image::getTransparencyCount() const {
	return FreeImage_GetTransparencyCount(_dib);
}

BYTE* Image::getTransparencyTable() const {
	return FreeImage_GetTransparencyTable(_dib);
}

void Image::setTransparencyTable(BYTE *table, int count) {
	FreeImage_SetTransparencyTable(_dib, table, count);
	_bHasChanged = true;
}
#endif

bool Image::hasFileBkColor() const {
	return FreeImage_HasBackgroundColor(_dib);
}

bool Image::getFileBkColor(RGBQUAD *bkcolor) const {
	return FreeImage_GetBackgroundColor(_dib, bkcolor);
}

bool Image::setFileBkColor(RGBQUAD *bkcolor) {
	_bHasChanged = true;
	return FreeImage_SetBackgroundColor(_dib, bkcolor);
}

///////////////////////////////////////////////////////////////////   
// Channel processing support

bool Image::getChannel(Image& image, FREE_IMAGE_COLOR_CHANNEL channel) const {
	if(_dib) {
		image = FreeImage_GetChannel(_dib, channel);
		return image.isValid();
	}
	return false;
}

bool Image::setChannel(const Image& image, FREE_IMAGE_COLOR_CHANNEL channel) {
	if(_dib) {
		_bHasChanged = true;
		return FreeImage_SetChannel(_dib, image._dib, channel);
	}
	return false;
}

bool Image::splitChannels(Image& RedChannel, Image& GreenChannel, Image& BlueChannel) const {
	if(_dib) {
		RedChannel = FreeImage_GetChannel(_dib, FICC_RED);
		GreenChannel = FreeImage_GetChannel(_dib, FICC_GREEN);
		BlueChannel = FreeImage_GetChannel(_dib, FICC_BLUE);

		return (RedChannel.isValid() && GreenChannel.isValid() && BlueChannel.isValid());
	}
	return false;
}

bool Image::combineChannels(const Image& red, const Image& green, const Image& blue) {
	if(!_dib) {
		int width = red.getWidth();
		int height = red.getHeight();
		_dib = FreeImage_Allocate(width, height, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
	}

	if(_dib) {
		bool bResult = true;
		bResult &= FreeImage_SetChannel(_dib, red._dib, FICC_RED);
		bResult &= FreeImage_SetChannel(_dib, green._dib, FICC_GREEN);
		bResult &= FreeImage_SetChannel(_dib, blue._dib, FICC_BLUE);

		_bHasChanged = true;

		return bResult;
	}
	return false;
}

///////////////////////////////////////////////////////////////////   
// Rotation and flipping

bool Image::rotateEx(double angle, double x_shift, double y_shift, double x_origin, double y_origin, bool use_mask) {
	if(_dib) {
		if(FreeImage_GetBPP(_dib) >= 8) {
			FIBITMAP *rotated = FreeImage_RotateEx(_dib, angle, x_shift, y_shift, x_origin, y_origin, use_mask);
			return replace(rotated);
		}
	}
	return false;
}

bool Image::rotate(double angle, const void *bkcolor) {
	if(_dib) {
		switch(FreeImage_GetImageType(_dib)) {
			case FIT_BITMAP:
				switch(FreeImage_GetBPP(_dib)) {
					case 1:
					case 8:
					case 24:
					case 32:
						break;
					default:
						return false;
				}
				break;

			case FIT_UINT16:
			case FIT_RGB16:
			case FIT_RGBA16:
			case FIT_FLOAT:
			case FIT_RGBF:
			case FIT_RGBAF:
				break;
			default:
				return false;
				break;
		}

		FIBITMAP *rotated = FreeImage_Rotate(_dib, angle, bkcolor);
		return replace(rotated);

	}
	return false;
}

bool Image::flipVertical() {
	if(_dib) {
		_bHasChanged = true;

		return FreeImage_FlipVertical(_dib);
	}
	return false;
}

bool Image::flipHorizontal() {
	if(_dib) {
		_bHasChanged = true;

		return FreeImage_FlipHorizontal(_dib);
	}
	return false;
}

///////////////////////////////////////////////////////////////////   
// Color manipulation routines

bool Image::invert() {
	if(_dib) {
		_bHasChanged = true;

		return FreeImage_Invert(_dib);
	}
	return false;
}

bool Image::adjustCurve(BYTE *LUT, FREE_IMAGE_COLOR_CHANNEL channel) {
	if(_dib) {
		_bHasChanged = true;

		return FreeImage_AdjustCurve(_dib, LUT, channel);
	}
	return false;
}

bool Image::adjustGamma(double gamma) {
	if(_dib) {
		_bHasChanged = true;

		return FreeImage_AdjustGamma(_dib, gamma);
	}
	return false;
}

bool Image::adjustBrightness(double percentage) {
	if(_dib) {
		_bHasChanged = true;

		return FreeImage_AdjustBrightness(_dib, percentage);
	}
	return false;
}

bool Image::adjustContrast(double percentage) {
	if(_dib) {
		_bHasChanged = true;

		return FreeImage_AdjustContrast(_dib, percentage);
	}
	return false;
}

bool Image::adjustBrightnessContrastGamma(double brightness, double contrast, double gamma) {
	if(_dib) {
		_bHasChanged = true;

		return FreeImage_AdjustColors(_dib, brightness, contrast, gamma, false);
	}
	return false;
}

bool Image::getHistogram(DWORD *histo, FREE_IMAGE_COLOR_CHANNEL channel) const {
	if(_dib) {
		return FreeImage_GetHistogram(_dib, histo, channel);
	}
	return false;
}

///////////////////////////////////////////////////////////////////
// Upsampling / downsampling routine

bool Image::rescale(unsigned new_width, unsigned new_height, FREE_IMAGE_FILTER filter, bool keepAspect) {
	if(_dib) {
		switch(FreeImage_GetImageType(_dib)) {
			case FIT_BITMAP:
			case FIT_UINT16:
			case FIT_RGB16:
			case FIT_RGBA16:
			case FIT_FLOAT:
			case FIT_RGBF:
			case FIT_RGBAF:
				break;
			default:
				return false;
				break;
		}
		if (keepAspect) {
			WORD nw = getWidth();
			WORD nh = getHeight();

			if (nw > new_width) {
				double r = (double)new_width / (double)nw;
				nw = new_width;
				nh = max(1, (int)fabs((double)nh * r));
			}
			if (nh > new_height) {
				double r = (double)new_height / (double)nh;
				nh = new_height;
				nw = max(1, (int)fabs((double)nw * r));
			}
			new_width = nw;
			new_height = nh;
		}

		// Perform upsampling / downsampling
		FIBITMAP *dst = FreeImage_Rescale(_dib, new_width, new_height, filter);
		return replace(dst);
	}
	return false;
}

bool Image::makeThumbnail(unsigned maxWidth, unsigned maxHeight, bool convert) {
	bool rv = false;
	if (maxHeight == 0) {
		maxHeight = maxWidth;
	}
	if(_dib && maxWidth && maxHeight) {
		WORD nw = getWidth();
		WORD nh = getHeight();
		if (nw < maxWidth && nh < maxHeight) {
			return true;
		}
		
		if (nw > maxWidth) {
			double r = (double)maxWidth / (double)nw;
			nw = maxWidth;
			nh = max(1, (int)fabs((double)nh * r));
		}
		if (nh > maxHeight) {
			double r = (double)maxHeight / (double)nh;
			nh = maxHeight;
			nw = max(1, (int)fabs((double)nw * r));
		}

		rv = rescale(nw, nh, FILTER_CATMULLROM);
		if (rv && convert) {
			convertToType(FIT_BITMAP);
		}
	}
	return rv;
}

///////////////////////////////////////////////////////////////////
// Metadata

unsigned Image::getMetadataCount(FREE_IMAGE_MDMODEL model) const {
	return FreeImage_GetMetadataCount(model, _dib);
}

bool Image::getMetadata(FREE_IMAGE_MDMODEL model, const std::string& key, Tag& tag) const {
	FITAG *searchedTag = 0;
	FreeImage_GetMetadata(model, _dib, key.c_str(), &searchedTag);
	if(searchedTag != 0) {
		tag = FreeImage_CloneTag(searchedTag);
		return true;
	} else {
		// clear the tag
		tag = (FITAG*)0;
	}
	return false;
}

bool Image::setMetadata(FREE_IMAGE_MDMODEL model, const  std::string& key, Tag& tag) {
	return FreeImage_SetMetadata(model, _dib, key.c_str(), tag);
}

