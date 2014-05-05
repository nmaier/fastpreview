#pragma once
#include <windows.h>

class FileAttr : public WIN32_FILE_ATTRIBUTE_DATA
{
private:
  __int64 size;

public:
  FileAttr(const std::wstring& aFile)
  {
    if (!GetFileAttributesEx(aFile.c_str(), GetFileExInfoStandard, this)) {
      throw WindowsException();
    }
    size = nFileSizeLow + nFileSizeHigh * MAXDWORD;
  }

  FileAttr(const FileAttr& rhs)
  {
    memcpy(this, &rhs, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
    size = rhs.size;
  }

  FileAttr& operator=(const FileAttr& rhs)
  {
    memcpy(this, &rhs, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
    size = rhs.size;
    return *this;
  }

  bool operator==(const FileAttr& rhs) const
  {
    return rhs.size == size &&
      rhs.ftLastWriteTime.dwLowDateTime == ftLastWriteTime.dwLowDateTime;
  }

  bool operator!=(const FileAttr& rhs) const
  {
    return !operator==(rhs);
  }

  __int64 getSize() const
  {
    return size;
  }
};
