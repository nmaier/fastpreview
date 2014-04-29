/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#include "TinyXMP.h"

#define TXMP_STRING_TYPE std::string
#include <XMP.incl_cpp>
#include <XMP.hpp>

#include <queue>
#include <sstream>

namespace {
  typedef struct _standard_xmp_t
  {
    const char *ns;
    const char *desc;
  } standard_xmp_t;

  standard_xmp_t standard_xmp[] = {
    { kXMP_NS_DC, "Dublin Core" },
    { kXMP_NS_XMP, "Main" },
    { kXMP_NS_XMP_Rights, "Rights Management" },
    { kXMP_NS_XMP_MM, "Media Management" },
    { kXMP_NS_XMP_BJ, "Basic Job Ticket" },
    { kXMP_NS_XMP_PagedFile, "Paged-text" },
    { kXMP_NS_DM, "Dynamic Media" },
    { kXMP_NS_PDF, "Adobe PDF" },
    { kXMP_NS_Photoshop, "Photoshop" },
    { kXMP_NS_CameraRaw, "Camera Raw" },
    { kXMP_NS_TIFF, "TIFF/Exif" },
    { kXMP_NS_EXIF, "Exif" },
    { kXMP_NS_EXIF_Aux, "Exif (Aux)" },
    { kXMP_NS_PSAlbum, "Photoshop Album" },
    { kXMP_NS_PNG, "Portable Network Graphic (PNG)" },
    { kXMP_NS_SWF, "Shockwave Flash (SWF)" },
    { kXMP_NS_JPEG, "Joint Picture Expert Group (JPEG)" },
    { kXMP_NS_JP2K, "JPEG 2000 (jp2)" },
    { kXMP_NS_ASF, "Advanced Streaming Format (ASF)" },
    { kXMP_NS_WAV, "Audio (WAV)" },
    { kXMP_NS_XMP_Note, "Notes" },
    { kXMP_NS_AdobeStockPhoto, "Adobe Stock Photo" },
    { kXMP_NS_CreatorAtom, "Creator Atom" },
    { kXMP_NS_XMP_IdentifierQual, "Identificator Qualifiers" },
    { kXMP_NS_XMP_Dimensions, "Dimensions" },
    { kXMP_NS_XMP_Text, "Text" },
    { kXMP_NS_XMP_Graphics, "Graphics" },
    { kXMP_NS_XMP_Image, "Image" },
    { kXMP_NS_XMP_Font, "Fonts" },
    { kXMP_NS_IPTCCore, "IPTC/NAA Core" },
    { kXMP_NS_DICOM, "Digital Imaging and Communications in Medicine" },
    { kXMP_NS_XML, "XML" },
    { "http://ns.camerabits.com/photomechanic/1.0/", "Camera Bits Photo Mechanic" },
    { "http://xmp.gettyimages.com/gift/1.0/", "Getty Images" },
  };

  static std::string sanitizePath(const std::string& path)
  {
    using std::string;

    std::queue<std::string> q;
    string::size_type p, o = 0;
    for (p = path.find('/'); p != string::npos;
      o = p, p = path.find('/', p + 1)) {
      q.push(path.substr(o, p));
    }
    q.push(path.substr(o));
    std::stringstream ss;
    while (!q.empty()) {
      string c = q.front();
      if ((p = c.rfind('[')) != string::npos) {
        c = c.substr(0, p);
      }
      if ((p = c.find(':')) != string::npos) {
        c = c.substr(p + 1);
      }
      ss << "/";
      for (const char *cb = c.c_str(), *ch = cb; *ch; ++ch) {
        if (ch == cb) {
          ss << (char)toupper(*ch);
        }
        else {
          ss << *ch;
        }
        if (*(ch + 1) && !islower(*(ch + 1)) && *(ch + 2) &&
          islower(*(ch + 2))) {
          ss << ' ';
        }
      }
      q.pop();
    }
    return ss.str().substr(1);
  }

};

namespace TinyXMP {

  using std::string;

  Namespace::Namespace(const std::string& aUrn, std::string& aDescription)
    : urn_(aUrn), description_(aDescription)
  {
    if (description_.empty()) {
      description_ = "Adobe XMP (" + urn_ + ")";
    }
  }

  void Namespace::add(const std::string& aName, const std::string& aValue)
  {
    entries_[aName].insert(aValue);
  }

  XMP::XMP()
  {
    SXMPMeta::Initialize();
  }
  XMP::~XMP()
  {
    SXMPMeta::Terminate();
  }


  void XMP::parse(const std::string& block)
  {
    try {
      if (!SXMPMeta::Initialize()) {
        throw Exception("Failed to initialize XMP library");
      }
      SXMPMeta xmp;

      typedef std::map<string, string> Descs;
      Descs descs;

      for (const auto& sns : standard_xmp) {
        xmp.RegisterStandardAliases(sns.ns);
        descs[sns.ns] = sns.desc;
      }

      xmp.ParseFromBuffer(block.c_str(), block.length());

      SXMPIterator itr(xmp, kXMP_IterJustLeafNodes);
      string urn, path, value;
      while (itr.Next(&urn, &path, &value)) {
        path = sanitizePath(path);
        string desc;
        Descs::const_iterator d = descs.find(urn);
        if (d != descs.end()) {
          desc = d->second;
        }
        Namespace ns(urn, desc);
        auto i = namespaces_.find(ns);
        if (i != namespaces_.end()) {
          Namespace &ins = const_cast<Namespace&>(*i);
          ins.add(path, value);
        }
        else {
          ns.add(path, value);
          namespaces_.insert(ns);
        }
      }
    }
    catch (XMP_Error &ex) {
      throw Exception(ex.GetErrMsg());
    }
  }

  void XMP::clear()
  {
    namespaces_.clear();
  }

}; // namespace