#include "resample.h"
#include "dptr.h"
#include "fwBase.h"
#include "fwImage.h"
#include "Exception.h"
#include "console.h"

using std::wstring;

typedef FwStatus (STDCALL *fwiResize_8u)( const Fw8u* pSrc, FwiSize srcSize, int srcStep, FwiRect srcRoi,	Fw8u* pDst, int dstStep, FwiSize dstRoiSize, double xFactor, double yFactor, int interpolation);

GFLC_BITMAP *gflResampleImageEx(GFLC_BITMAP *aBitmap, GFL_RESAMPLE_METHOD method, const GFL_UINT32 aWidth, const GFL_UINT32 aHeight)
{
	ConWrite(itos(method));
	fwiResize_8u resize;
	const unsigned step = aBitmap->getComponentsPerPixel();
	switch (aBitmap->getType())
	{
	case GFL_GREY:
		resize = fwiResize_8u_C1R;
		break;
	case GFL_RGB:
	case GFL_BGR:
		resize = fwiResize_8u_C3R;
		break;
	case GFL_RGBA:
	case GFL_BGRA:
		resize = fwiResize_8u_AC4R;
		break;
	case GFL_CMYK:
		resize = fwiResize_8u_C4R;
		break;
	default:
		throw Exception(_T("Wrong ColorModel"));
	}

	GFLC_BITMAP* d = new GFLC_BITMAP(aBitmap->getType(), aWidth, aHeight, 1);

	FwiSize srcSize = { aBitmap->getWidth(), aBitmap->getHeight() };
	FwiSize dstSize = { d->getWidth(), d->getHeight() };
	FwiRect srcRoi = { 0, 0, srcSize.width, srcSize.height };
	double factor =  double(dstSize.width) / double(srcSize.width);
	if (factor > 1 && method == SUPER)
	{
		method = CUBIC;
	}
	FwStatus status = resize(
		(const Fw8u*)aBitmap->getDataPtr(),
		srcSize,
		step * aBitmap->getWidth(),
		srcRoi,
		(Fw8u*)d->getDataPtr(),
		step * d->getWidth(),
		dstSize,
		factor,
		factor,
		method
		);
	if (status != fwStsNoErr)
	{
		throw Exception(fwGetStatusString(status));
	}
	ConWrite(StringToWString(fwGetStatusString(status)));
	return d;
}