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

#ifndef KML_XSD_XSD_COMPLEX_TYPE_H__
#define KML_XSD_XSD_COMPLEX_TYPE_H__

#include <vector>
#include "boost/intrusive_ptr.hpp"
#include "kml/base/attributes.h"
#include "kml/xsd/xsd_element.h"
#include "kml/xsd/xsd_type.h"

namespace kmlxsd {

class XsdComplexType;

// Use this typedef to manage the XsdComplexType pointer.  For example:
//   XsdComplexTypePtr complex_type = XsdComplexType::Create(attributes);
typedef boost::intrusive_ptr<XsdComplexType> XsdComplexTypePtr;

// Corresponds to <xs:complexType> with possible <xs:extension> and use of
// <xs:sequence> (order of <xs:element>'s matters in <xs:sequence>).
class XsdComplexType : public XsdType {
 public:
  // Create an XsdComplexType from the given attributes.  The "name" attribute
  // must exist for this to succeed.  On success a pointer is returned which
  // may be managed with intrusive_ptr using the recommended typedef above.
  static XsdComplexType* Create(const kmlbase::Attributes& attributes) {
    string name;
    if (attributes.GetString("name", &name)) {
      return new XsdComplexType(name);
    }
    return NULL;
  }

  // This dynamic cast to XsdComplexTypePtr returns non-NULL if the xsd_type
  // is non-NULL and is_complex() is true.
  static XsdComplexTypePtr AsComplexType(const XsdTypePtr& xsd_type) {
    if (xsd_type && xsd_type->get_xsd_type_id() == XSD_TYPE_COMPLEX) {
      return boost::static_pointer_cast<XsdComplexType>(xsd_type);
    }
    return NULL;
  }

  virtual XsdTypeEnum get_xsd_type_id() const {
    return XSD_TYPE_COMPLEX;
  }

  virtual bool is_complex() const {
    return true;
  }

  // Get the value of the name attribute.
  virtual const string get_name() const {
    return name_;
  }

  virtual const string get_base() const {
    return extension_base_;
  }

  // Set the value of the "base" attribute of the complexType's
  // <xs:extension> element.
  void set_extension_base(const string& extension_base) {
    extension_base_ = extension_base;
  }
  // Get the <xs:extension base=".."> value.
  const string& get_extension_base() const {
    return extension_base_;
  }
  // Return true IFF this complexType has an <xs:extension base="..."/>.
  bool has_extension_base() const {
    return !extension_base_.empty();
  }

  // Append the given <xs:element> to this complexType's <xs:sequence>.
  void add_element(const XsdElementPtr& element) {
    sequence_.push_back(element);
  }

  // Return the number of elements in the <xs:sequence>.
  size_t get_sequence_size() const {
    return sequence_.size();
  }
  // Return the index'th element in the <xs:sequence>.
  const XsdElementPtr get_sequence_at(size_t index) const {
    return sequence_[index];
  }

 private:
  bool ParseAttributes(const kmlbase::Attributes& attributes);
  XsdComplexType(const string& name)
    : name_(name) {
  }

  string name_;
  string extension_base_;  // <xs:extension base="xx">
  std::vector<XsdElementPtr> sequence_;  // <xs:sequence> of <xs:element>'s.
};

}  // end namespace kmlxsd

#endif  // KML_XSD_XSD_COMPLEX_TYPE_H__
