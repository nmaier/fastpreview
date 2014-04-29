#pragma once

#include <windows.h>
#include <gflc.h>

typedef enum {
	NEAREST	= 1,
	LINEAR	= 2,
	CUBIC	= 4,
	SUPER	= 8,
	LANCZOS	= 16
} GFL_RESAMPLE_METHOD;

GFLC_BITMAP *gflResampleImageEx(GFLC_BITMAP *aBmp, GFL_RESAMPLE_METHOD method, const GFL_UINT32 aWidth, const GFL_UINT32 aHeight);