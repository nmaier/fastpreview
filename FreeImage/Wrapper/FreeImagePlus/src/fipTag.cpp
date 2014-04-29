// ==========================================================
// Tag class implementation
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

#include <string.h>
#include "FreeImagePlus.h"

using namespace FreeImage;

Tag::Tag() {
	_tag = FreeImage_CreateTag();
}

Tag::~Tag() {
	FreeImage_DeleteTag(_tag);
}

bool Tag::setKeyValue(const std::string& key, const std::string& value) {
	if(_tag) {
		FreeImage_DeleteTag(_tag);
		_tag = NULL;
	}
	// create a tag
	_tag = FreeImage_CreateTag();
	if(_tag) {
		bool bSuccess = true;
		// fill the tag
		DWORD tag_length = (DWORD)(value.size() + 1);
		bSuccess &= FreeImage_SetTagKey(_tag, key.c_str());
		bSuccess &= FreeImage_SetTagLength(_tag, tag_length);
		bSuccess &= FreeImage_SetTagCount(_tag, tag_length);
		bSuccess &= FreeImage_SetTagType(_tag, FIDT_ASCII);
		bSuccess &= FreeImage_SetTagValue(_tag, value.c_str());
		return bSuccess;
	}
	return false;
}

Tag::Tag(const Tag& tag) {
	_tag = FreeImage_CloneTag(tag._tag);
}

Tag& Tag::operator=(const Tag& tag) {
	if(this != &tag) {
		if(_tag) FreeImage_DeleteTag(_tag);
		_tag = FreeImage_CloneTag(tag._tag);
	}
	return *this;
}

Tag& Tag::operator=(FITAG *tag) {
	if(_tag) FreeImage_DeleteTag(_tag);
	_tag = tag;
	return *this;
}

bool Tag::isValid() const {
	return (_tag != nullptr) ? true : false;
}

std::string Tag::getKey() const {
	const char *v = FreeImage_GetTagKey(_tag);
	return v ? v : "";
}

std::string Tag::getDescription() const {
	const char *v = FreeImage_GetTagDescription(_tag);
	return v ? v : "";
}

WORD Tag::getID() const {
	return FreeImage_GetTagID(_tag);
}

FREE_IMAGE_MDTYPE Tag::getTagType() const {
	return FreeImage_GetTagType(_tag);
}

DWORD Tag::getCount() const {
	return FreeImage_GetTagCount(_tag);
}

DWORD Tag::getLength() const {
	return FreeImage_GetTagLength(_tag);
}

const void* Tag::getValue() const {
	return FreeImage_GetTagValue(_tag);
}

bool Tag::setKey(const std::string& key) {
	return FreeImage_SetTagKey(_tag, key.c_str());
}

bool Tag::setDescription(const std::string& description) {
	return FreeImage_SetTagDescription(_tag, description.c_str());
}

bool Tag::setID(WORD id) {
	return FreeImage_SetTagID(_tag, id);
}

bool Tag::setType(FREE_IMAGE_MDTYPE type) {
	return FreeImage_SetTagType(_tag, type);
}

bool Tag::setCount(DWORD count) {
	return FreeImage_SetTagCount(_tag, count);
}

bool Tag::setLength(DWORD length) {
	return FreeImage_SetTagLength(_tag, length);
}

bool Tag::setValue(const void *value) {
	return FreeImage_SetTagValue(_tag, value);
}

std::string Tag::toString(FREE_IMAGE_MDMODEL model, char *Make) const {
	return FreeImage_TagToString(model, _tag, Make);
}
