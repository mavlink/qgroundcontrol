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

#ifndef KML_XSD_XSD_TYPE_H__
#define KML_XSD_XSD_TYPE_H__

#include "boost/intrusive_ptr.hpp"
#include "kml/base/referent.h"
#include "kml/base/util.h"

namespace kmlxsd {

// This is a pure virtual base type for all other XSD type types such as
// <xs:simpleType> and <xs:complexType>.
class XsdType : public kmlbase::Referent {
 public:
  typedef enum {
    XSD_TYPE_PRIMITIVE,
    XSD_TYPE_SIMPLE,
    XSD_TYPE_COMPLEX
  } XsdTypeEnum;

  virtual XsdTypeEnum get_xsd_type_id() const = 0;

  virtual ~XsdType() {}

  // This returns true of this is an <xs:complexType>.
  virtual bool is_complex() const = 0;

  // This returns the name attribute of an <xs:simpleType> or <xs:complexType>
  // and the XSD name for a primitive type, for example "string", "boolean"
  // or "double".
  virtual const string get_name() const = 0;

  // This returns the extension base for a <xs:complexType>, the restriction
  // base for a <xs:simpleType> and "xsd:primitive" for a primitive type.
  virtual const string get_base() const = 0;

  // Two XsdType's are equal if their names are the same.
  bool operator==(const XsdType& xsd_type) const {
    return get_name() == xsd_type.get_name();
  }

};

typedef boost::intrusive_ptr<XsdType> XsdTypePtr;

}  // end namespace kmlxsd

#endif  // KML_XSD_XSD_TYPE_H__
