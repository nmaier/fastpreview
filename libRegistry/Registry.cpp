#include "Registry.h"

Registry::Registry(HKEY hkey, const std::wstring& key)
: hkey_(hkey), key_(key), subkey_(nullptr)
{
  if (key_.empty()) {
    throw std::logic_error("key cannot be empty");
  }
}

Registry::~Registry()
{
  if (subkey_) {
    RegCloseKey(subkey_);
    subkey_ = nullptr;
  }
}

bool Registry::open() const
{
  if (subkey_) {
    RegCloseKey(subkey_);
    subkey_ = nullptr;
  }
  return ::RegOpenKeyEx(hkey_, key_.c_str(), 0, KEY_ALL_ACCESS, &subkey_) ==
    ERROR_SUCCESS;
}

bool Registry::create() const
{
  if (subkey_) {
    RegCloseKey(subkey_);
    subkey_ = nullptr;
  }
  DWORD dispo;
  return ::RegCreateKeyEx(
    hkey_,
    key_.c_str(),
    0,
    nullptr,
    REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS,
    nullptr,
    &subkey_,
    &dispo
    ) == ERROR_SUCCESS;
}

bool Registry::remove() const
{
  return RegDeleteTree(hkey_, key_.c_str()) == ERROR_SUCCESS;
}

bool Registry::unset(const std::wstring& name) const
{
  if (!subkey_) {
    throw std::logic_error("key not open");
  }
  if (!exists(name)) {
    return true;
  }
  return RegDeleteKeyValue(subkey_, nullptr, name.c_str()) == ERROR_SUCCESS;
}

bool Registry::exists(const std::wstring& name) const
{
  if (!subkey_) {
    throw std::logic_error("key not open");
  }
  DWORD size = 0;
  auto rv = RegGetValue(
    subkey_,
    nullptr,
    name.c_str(),
    RRF_RT_ANY,
    nullptr,
    0,
    &size);
  return rv == ERROR_SUCCESS || rv == ERROR_MORE_DATA;
}

bool Registry::get(const std::wstring& name, std::wstring& data) const
{
  if (!subkey_) {
    throw std::logic_error("key not open");
  }
  DWORD size = 0;
  auto rv = RegGetValue(
    subkey_,
    nullptr,
    name.c_str(),
    RRF_RT_REG_SZ | RRF_RT_REG_BINARY | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,
    nullptr,
    nullptr,
    &size
    );
  if (rv != ERROR_SUCCESS) {
    return false;
  }
  if (size == 0) {
    return true;
  }
  if (size % 2) {
    // Cannot convert.
    return false;
  }
  auto buf = std::make_unique<wchar_t[]>(size / sizeof(wchar_t));
  rv = RegGetValue(
    subkey_,
    nullptr,
    name.c_str(),
    RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,
    nullptr,
    buf.get(),
    &size
    );
  if (rv != ERROR_SUCCESS) {
    return false;
  }
  data = std::wstring(buf.get(), (size - 1) / sizeof(wchar_t));
  return true;
}

bool Registry::get(const std::wstring& name, uint32_t& data) const
{
  if (!subkey_) {
    throw std::logic_error("key not open");
  }
  DWORD size = sizeof(uint32_t);
  auto rv = RegGetValue(
    subkey_,
    nullptr,
    name.c_str(),
    RRF_RT_DWORD,
    nullptr,
    &data,
    &size
    );
  return rv == ERROR_SUCCESS;
}

bool Registry::get(const std::wstring& name, uint64_t& data) const
{
  if (!subkey_) {
    throw std::logic_error("key not open");
  }
  DWORD size = sizeof(uint64_t);
  auto rv = RegGetValue(
    subkey_,
    nullptr,
    name.c_str(),
    RRF_RT_QWORD,
    nullptr,
    &data,
    &size
    );
  return rv == ERROR_SUCCESS;
}

bool Registry::set(const std::wstring& name, const std::wstring& data, DWORD type) const
{
  if (!subkey_) {
    return false;
  }
  return RegSetValueEx(
    subkey_,
    name.c_str(),
    0,
    type,
    (BYTE*)data.c_str(),
    (data.length() + 1) * sizeof(wchar_t)
    ) == ERROR_SUCCESS;
}

bool Registry::set(const std::wstring& name, const uint32_t data, DWORD type) const
{
  if (!subkey_) {
    return false;
  }
  return ::RegSetValueEx(
    subkey_,
    name.c_str(),
    0,
    type,
    (BYTE*)&data,
    sizeof(uint32_t)
    ) == ERROR_SUCCESS;
}

bool Registry::set(const std::wstring& name, const uint64_t data, DWORD type) const
{
  if (!subkey_) {
    return false;
  }
  return ::RegSetValueEx(
    subkey_,
    name.c_str(),
    0,
    type,
    (BYTE*)&data,
    sizeof(uint64_t)
    ) == ERROR_SUCCESS;
}
