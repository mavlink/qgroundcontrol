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

// This file contains the definition of the XmlElement class.

#ifndef KML_BASE_XML_ELEMENT_H__
#define KML_BASE_XML_ELEMENT_H__

#include "boost/intrusive_ptr.hpp"
#include "kml/base/referent.h"
#include "kml/base/util.h"
#include "kml/base/xml_namespaces.h"

namespace kmlbase {

class XmlFile;

// Forward declare XmlElement to create typedef used within class XmlElement.
class XmlElement;

typedef boost::intrusive_ptr<XmlElement> XmlElementPtr;

// This class represents an XML element.  An XmlElement may be in one XmlFile,
// and may have one parent XmlElement.  This class is derived from Referent
// such that derived classes can use boost::intrusive_ptr.
class XmlElement : public Referent {
 public:
  // Get the parent XmlElement if any.
  const XmlElement* GetParent() const {
    return parent_;
  }

  // Get the parent XmlFile if any.
  const XmlFile* GetXmlFile() const {
    return xml_file_;
  }

  XmlnsId get_xmlns() const {
    return xmlns_id_;
  }

  // This returns true if the passed element is in the same XmlFile or if both
  // this XmlElement and the passed element are in no XmlFile.  Passing a NULL
  // pointer always causes a false return value.
  bool InSameXmlFile(const XmlElementPtr& element) const {
    return element && xml_file_ == element->xml_file_;
  }

  // If this element is not already in an XmlFile this associates this element
  // with the given XmlFile and true is returned.  If this element is already
  // in an XmlFile false is returned and that association remains.  There is
  // no means to detach an XmlElement from an XmlFile.
  bool SetXmlFile(const XmlFile* xml_file) {
    if (!xml_file_ && xml_file) {
      xml_file_ = xml_file;
      return true;
    }
    return false;
  }

 protected:
  // This is an abstract base class and is never created directly.
  XmlElement() : xmlns_id_(XMLNS_NONE), parent_(NULL), xml_file_(NULL) {}

  void set_xmlns(XmlnsId xmlns_id) {
    xmlns_id_ = xmlns_id;
  }

  // Only a derived class can set its parent.  This returns false if this
  // XmlElement already has a parent or if this XmlElement is in a different
  // XmlFile.
  bool SetParent(const XmlElementPtr& parent) {
    if (!parent_ && parent && InSameXmlFile(parent)) {
      parent_ = parent.get();
      return true;
    }
    return false;
  }

 private:
  XmlnsId xmlns_id_;
  const XmlElement* parent_;  // Can't ref count due to circularity.
  const XmlFile* xml_file_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(XmlElement);
};

}  // end namespace kmlbase

#endif // KML_BASE_XML_ELEMENT_H__
