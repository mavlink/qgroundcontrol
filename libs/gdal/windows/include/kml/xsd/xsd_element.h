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

// This file contains the declaration of the XsdElement class.

#ifndef KML_XSD_XSD_ELEMENT_H__
#define KML_XSD_XSD_ELEMENT_H__

#include "boost/intrusive_ptr.hpp"
#include "kml/base/referent.h"
#include "kml/xsd/xsd_primitive_type.h"

namespace kmlbase {
class Attributes;
}

namespace kmlxsd {

// XsdElement corresponds to <xs:element name="..." type="..." ... />
// or <xs:element ref="..."/>.
class XsdElement : public kmlbase::Referent {
public:
  // Create an XsdElement from the given attributes.
  static XsdElement* Create(const kmlbase::Attributes& attributes);

  // Get the value of the <xs:element abstract="..."> attribute.
  bool is_abstract() const {
    return abstract_;
  }

  // Get the value of the <xs:element default="..."> attribute.
  const string& get_default() const {
    return default_;
  }

  // Get the value of the <xs:element name="..."> attribute.  This is the
  // value of ref= if is_ref() is true.
  const string& get_name() const {
    return name_;
  }

  // Get the <xs:element substitutionGroup="..."> attribute.
  const string& get_substitution_group() const {
    return substitution_group_;
  }

  // Get the <xs:element type="..."> attribute.
  const string& get_type() const {
    return type_;
  }

  // Return the XSD type id of the element type IF this element is of a
  // primitive type, else return XSD_INVALID.
  XsdPrimitiveType::TypeId get_type_id() const {
    return type_id_;
  }

  // This returns true if the element is of an XSD native/primitive type.
  // Note that an element of any simpleType is _not_ considered primitive.
  // See XsdPrimitiveType for the list of XSD primitive types.
  bool is_primitive() const {
    return type_id_ != XsdPrimitiveType::XSD_INVALID;
  }

  // This returns true if this is an <xs:element ref="..."> and false if this
  // is an <xs:element name="..." ... >
  bool is_ref() const {
    return ref_ == true;
  }

 private:
  // Use static Create().
  XsdElement();
  // Set the class internals from the attributes and return true if the
  // attributes were valid for an <xsd:element>.  False is returned if there
  // was no "name" or "ref" attribute found.
  bool ParseAttributes(const kmlbase::Attributes& attributes);
  bool abstract_;
  bool ref_;
  int min_occurs_;
  int max_occurs_;
  string default_;
  string name_;
  string type_;
  XsdPrimitiveType::TypeId type_id_;
  string substitution_group_;
};

typedef boost::intrusive_ptr<XsdElement> XsdElementPtr;

}  // end namespace kmlxsd

#endif  // KML_XSD_XSD_ELEMENT_H__
