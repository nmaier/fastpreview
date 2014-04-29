// ==========================================================
// MetadataFind class implementation
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

bool MetadataFind::isValid() const {
	return (_mdhandle != NULL) ? true : false;
}

MetadataFind::MetadataFind() : _mdhandle(NULL) {
}

MetadataFind::~MetadataFind() {
	FreeImage_FindCloseMetadata(_mdhandle);
}

bool MetadataFind::findFirstMetadata(FREE_IMAGE_MDMODEL model, const Image& image, Tag& tag) {
	FITAG *firstTag = NULL;
	if(_mdhandle) FreeImage_FindCloseMetadata(_mdhandle);
	_mdhandle = FreeImage_FindFirstMetadata(model, image, &firstTag);
	if(_mdhandle) {
		tag = FreeImage_CloneTag(firstTag);
		return true;
	}
	return false;
} 

bool MetadataFind::findNextMetadata(Tag& tag) {
	FITAG *nextTag = NULL;
	if( FreeImage_FindNextMetadata(_mdhandle, &nextTag) ) {
		tag = FreeImage_CloneTag(nextTag);
		return true;
	}
	return false;
}

