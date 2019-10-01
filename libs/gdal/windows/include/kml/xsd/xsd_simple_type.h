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

#ifndef KML_XSD_XSD_SIMPLE_TYPE_H__
#define KML_XSD_XSD_SIMPLE_TYPE_H__

#include <vector>
#include "boost/intrusive_ptr.hpp"
#include "kml/base/attributes.h"
#include "kml/xsd/xsd_type.h"

namespace kmlxsd {

class XsdSimpleType;

typedef boost::intrusive_ptr<XsdSimpleType> XsdSimpleTypePtr;

// Corresponds to <xs:simpleType>.
class XsdSimpleType : public XsdType {
 public:
  static XsdSimpleType* Create(const kmlbase::Attributes& attributes) {
    string name;
    if (attributes.GetString("name", &name)) {
      return new XsdSimpleType(name);
    }
    return NULL;
  }

  static XsdSimpleTypePtr AsSimpleType(const XsdTypePtr& xsd_type) {
    if (xsd_type && xsd_type->get_xsd_type_id() == XSD_TYPE_SIMPLE) {
      return boost::static_pointer_cast<XsdSimpleType>(xsd_type);
    }
    return NULL;
  }

  virtual XsdTypeEnum get_xsd_type_id() const {
    return XSD_TYPE_SIMPLE;
  }

  virtual bool is_complex() const {
    return false;
  }

  // <xs:simpleType name="NAME"/>
  virtual const string get_name() const {
    return name_;
  }

  virtual const string get_base() const {
    return restriction_base_;
  }

  // <xs:restriction base="BASE"/>
  void set_restriction_base(const string& base) {
    restriction_base_ = base;
  }
  const string& get_restriction_base() const {
    return restriction_base_;
  }

  // <xs:enumeration value="VALUE"/>
  void add_enumeration(const string& value) {
    enumeration_.push_back(value);
  }

  // Return the number of <xs:enumeration value="..."/>'s.
  size_t get_enumeration_size() const {
    return enumeration_.empty() ? 0 : enumeration_.size();
  }

  // Return the index'th <xs:enumeration value="..."/>.  The order is preserved
  // as added in add_enumeration_value().
  const string& get_enumeration_at(size_t index) const {
    return enumeration_[index];
  }

  // Returns true if this is an enumerated type.
  bool IsEnumeration() const {
    return restriction_base_ == "string" && !enumeration_.empty();
  }

 private:
  // Client code should use Create().
  XsdSimpleType(const string& name)
    : name_(name) {
  }
  const string name_;
  string restriction_base_;
  std::vector<string> enumeration_;
};

}  // end namespace kmlxsd

#endif  // KML_XSD_XSD_SIMPLE_TYPE_H__
