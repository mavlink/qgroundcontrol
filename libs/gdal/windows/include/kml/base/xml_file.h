// Copyright 2008, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file contains the implementation of the XmlFile class.

#ifndef KML_BASE_XML_FILE_H__
#define KML_BASE_XML_FILE_H__

#include <map>
#include "boost/intrusive_ptr.hpp"
#include "kml/base/xml_element.h"
#include "kml/base/referent.h"
#include "kml/base/util.h"

namespace kmlbase {

// TODO: use a typedef (or type) for XmlId
typedef std::map<string, XmlElementPtr> XmlElementIdMap;

// This class represents an XML file (in XML standards this is known as a
// "document", however we avoid that term due to the use of "<Document>" as
// an element in KML).  An XmlFile may have a URL, a root XmlElement and
// a set of xml ID to XmlElement mappings.
class XmlFile : public Referent {
 public:
  const string& get_url() const {
    return url_;
  }

  const XmlElementPtr& get_root() const {
    return root_;
  }

 protected:
  void set_url(const string& url) {
    url_ = url;
  }

  bool set_root(const XmlElementPtr& element) {
    return root_ ? false : (root_ = element, true);
  }

  XmlElementPtr FindXmlElementById(const string& id) const {
    XmlElementIdMap::const_iterator find = id_map_.find(id);
    return find != id_map_.end() ? find->second : NULL;
  }

 protected:
  // This is an abstract base class and is never created directly.
  XmlFile() {}

 private:
  // TODO: use a typedef for URL and/or URL string
  string url_;
  XmlElementPtr root_;
  XmlElementIdMap id_map_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(XmlFile);
};

}  // end namespace kmlbase

#endif // KML_BASE_XML_FILE_H__
