/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#ifndef TINYXMP_H

#include <string>
#include <set>
#include <map>
#include <list>

namespace TinyXMP {

  class Namespace;
  class XMP;

  class Exception : public std::exception
  {
  public:
    Exception(const std::string& msg) : std::exception(msg.c_str())
    {}
  };

  typedef std::set<std::string> values_t;
  typedef std::map<std::string, values_t> entries_t;

  class Namespace
  {
    friend class XMP;

  private:
    std::string urn_;
    std::string description_;
    entries_t entries_;

    Namespace(const std::string& aUrn, std::string& aDescription);

    void add(const std::string& aName, const std::string& aValue);

  public:
    Namespace(const Namespace& rhs)
      : urn_(rhs.urn_), description_(rhs.description_)
    {}

    Namespace& operator=(const Namespace& rhs)
    {
      urn_ = rhs.urn_;
      description_ = rhs.description_;
      return *this;
    }
    bool operator<(const Namespace& rhs) const
    {
      return description_.compare(rhs.description_) < 0;
    }
    operator const std::string&() const
    {
      return description_;
    }
    operator const char*() const
    {
      return description_.c_str();
    }

    const std::string& getUrn() const
    {
      return urn_;
    }

    const std::string& getDescription() const
    {
      return description_;
    }

    const entries_t& getEntries() const
    {
      return entries_;
    }
  };

  typedef std::set<Namespace> namespaces_t;

  class XMP
  {
  private:
    namespaces_t namespaces_;

  public:
    XMP();
    ~XMP();
    void parse(const std::string& block);
    void clear();
    operator const namespaces_t&() const
    {
      return namespaces_;
    }
    const namespaces_t& getNamespaces() const
    {
      return namespaces_;
    }
  };

}; // namespace
#endif