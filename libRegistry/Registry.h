#pragma once

#include <windows.h>
#include <string>
#include <memory>

class Registry
{
private:
  HKEY hkey_;
  const std::wstring key_;
  mutable HKEY subkey_;

public:
  Registry(HKEY hkey, const std::wstring& key);
  ~Registry();

  bool open() const;

  bool create() const;

  bool remove() const;

  bool unset(const std::wstring& name) const;

  bool exists(const std::wstring& name) const;

  bool get(const std::wstring& name, std::wstring& data) const;

  bool get(const std::wstring& name, uint32_t& data) const;

  bool get(const std::wstring& name, uint64_t& data) const;

  inline bool get(const std::wstring& name, int32_t& data) const
  {
    return get(name, (uint32_t&)data);
  }

  inline bool get(const std::wstring& name, int64_t& data) const
  {
    return get(name, (uint64_t&)data);
  }

  inline bool get(const std::wstring& name, bool& data) const
  {
    uint32_t v = 0;
    if (get(name, v)) {
      data = v ? true : false;
      return true;
    }
    return false;
  }

  bool set(const std::wstring& name, const std::wstring& data, DWORD type = REG_SZ) const;

  bool set(const std::wstring& name, const uint32_t data, DWORD type = REG_DWORD) const;

  bool set(const std::wstring& name, const uint64_t data, DWORD type = REG_QWORD) const;

  inline bool set(const std::wstring& name, const bool data) const
  {
    uint32_t v = data ? 1 : 0;
    return set(name, v);
  }

  static bool exists(HKEY hkey, const std::wstring& key, const std::wstring& name = L"")
  {
    Registry r(hkey, key);
    if (!r.open()) {
      return false;
    }
    return r.exists(name);
  }

  template<typename T>
  static bool get(HKEY hkey, const std::wstring& key, const std::wstring& name, T& data)
  {
    Registry r(hkey, key);
    if (!r.open()) {
      return false;
    }
    return r.get(name, data);
  }

  template<typename T>
  static bool set(HKEY hkey, const std::wstring& key, const std::wstring& name, T data, DWORD type = REG_SZ)
  {
    Registry r(hkey, key);
    if (!r.create()) {
      return false;
    }
    return r.set(name, data, type);
  }

  static bool remove(HKEY hkey, const std::wstring& key)
  {
    Registry r(hkey, key);
    return r.remove();
  }

  static bool unset(HKEY hkey, const std::wstring& key, const std::wstring& name)
  {
    Registry r(hkey, key);
    if (!r.open()) {
      return true;
    }
    return r.unset(name);
  }

};