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

// This file declares the XsdPrimitiveType.

#ifndef KML_XSD_XSD_PRIMITIVE_TYPE_H__
#define KML_XSD_XSD_PRIMITIVE_TYPE_H__

#include "kml/xsd/xsd_type.h"

namespace kmlxsd {

// This class is a specialization of XsdType for XSD primitive ("built-in")
// types.  This permits elements of non-complexType and non-simpleType to have
// an XsdType.  This class also has methods to convert between type name as
// found in an XSD file and the enum defined here.
class XsdPrimitiveType : public XsdType {
 public:
  // This enumerates the XSD primitive ("built-in") types as listed here:
  //   http://www.w3.org/TR/xmlschema-2/#built-in-primitive-datatypes
  enum TypeId {
    XSD_INVALID,
    XSD_STRING,  // 3.2.1
    XSD_BOOLEAN,  // 3.2.2
    XSD_DECIMAL,  // 3.2.3
    XSD_FLOAT,  // 3.2.4
    XSD_DOUBLE,  // 3.2.5
    XSD_DURATION,  // 3.2.6
    XSD_DATE_TIME,  // 3.2.7
    XSD_TIME,  // 3.2.8
    XSD_DATE,  // 3.2.9
    XSD_G_YEAR_MONTH,  // 3.2.10
    XSD_G_YEAR,  // 3.2.11
    XSD_G_MONTH_DAY,  // 3.2.12
    XSD_G_DAY,  // 3.2.13
    XSD_G_MONTH,  // 3.2.14
    XSD_HEX_BINARY,  // 3.2.15
    XSD_BASE64_BINARY,  // 3.2.16
    XSD_ANY_URI,  // 3.2.17
    XSD_QNAME,  // 3.2.18
    XSD_NOTATION,  // 3.2.19
    // NOTE: this includes only the types involved in the xsd:int derivation.
    XSD_INTEGER,  // 3.3.13.  Is-a XSD_DECIMAL.
    XSD_LONG,  // 3.3.16.  Is-a XSD_INTEGER.
    XSD_INT  // 3.3.17.  Is-a XSD_LONG.
  };

  // Use this method to create a new XsdPrimitiveType from the type name.
  // For example the XsdPrimitiveType for <element name="open" type="boolean">
  // would be:
  // XsdPrimitiveTypePtr primitive_type = XsdPrimitiveType::Create("boolean");
  // If the type_name is not that of an XSD primitive type then no
  // XsdPrimitiveType is created and NULL is returned.
  static XsdPrimitiveType* Create(const string& type_name) {
    TypeId type_id = GetTypeId(type_name);
    if (type_id != XSD_INVALID) {
      return new XsdPrimitiveType(type_id);
    }
    return NULL;
  }

  virtual XsdTypeEnum get_xsd_type_id() const {
    return XSD_TYPE_PRIMITIVE;
  }

  // The XsdPrimitiveType implementation of this XsdType virtual method always
  // returns false.
  virtual bool is_complex() const {
    return false;
  }

  // The XsdPrimitiveType implementation of this XsdType virtual method returns
  // the name of the type.
  virtual const string get_name() const {
    return GetTypeName(type_id_);
  }

  // The XsdPrimitiveType implementation of XsdType returns "xsd:primitive".
  virtual const string get_base() const {
    return "xsd:primitive";
  }

  // This returns the name of the given XSD primitive type.  For example,
  // XSD_DOUBLE returns "double.  An empty string is returned for an invalid
  // type id.
  static const string GetTypeName(TypeId type_id);

  // This returns the id of the given XSD primitive type.  For example,
  // "double" returns XSD_DOUBLE.  XSD_INVALID is returned if the name is
  // on that of an XSD primitive type.
  static XsdPrimitiveType::TypeId GetTypeId(const string& type_name);

 private:
  // Use the static Create method to create an XsdPrimitiveType.
  XsdPrimitiveType(TypeId type_id)
    : type_id_(type_id) {
  }
  TypeId type_id_;
};

}  // end namespace kmlxsd

#endif  // KML_XSD_XSD_ELEMENT_H__
