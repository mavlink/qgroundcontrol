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

// This file declares the internal Xsd class which holds some of the
// information in the KML XSD.  There are three main users of the Xsd class:
// 1) the parser for mapping names to ids, 2) the dom for mapping ids
// to classes, and 3) the serializer for mapping ids back to names.

#ifndef KML_XSD_XSD_H__
#define KML_XSD_XSD_H__

#include <map>
#include "kml/base/util.h"

namespace kmldom {

enum XsdType {
  XSD_SIMPLE_TYPE,
  XSD_COMPLEX_TYPE,
  XSD_UNKNOWN
};

// This represents an XSD <element>.
// For example:
//  <element name="name" type="string"/>
//  If the type is fundamental then xsd_type_t is XSD_SIMPLE_TYPE
//  Enumerations are considered simple.
// Or:
//  <element name="Placemark" type="kml:PlacemarkType"
//    substitutionGroup="kml:AbstractFeatureGroup"/>
//  If the type is no fundamental then xsd_type_t is XSD_COMPLEX_TYPE
struct XsdElement {
  const char* element_name_;  // <element name="element_name_" ...  />
  XsdType xsd_type_;  // <element ... type="simple-or-complex" />
};

// This respresents an XSD <simpleType> with <restriction base="string">
// and <enumeration> children.  For example:
//  <simpleType name="altitudeModeEnumType">
//    <restriction base="string">
//      <enumeration value="clampToGround"/>
//      <enumeration value="relativeToGround"/>
//      <enumeration value="absolute"/>
//    </restriction>
//  </simpleType>
// KML's use of XSD offers an opportunity for simplification: there is
// a 1:1 mapping between the element name and the type.  That is, altitudeMode
// is the only element of type altitudeModeEnumType and vice versa.
struct XsdSimpleTypeEnum {
  // Yes, this is the element of this type not the type itself
  int type_id;
  const char** enum_value_list;  // Value of value attribute.
};

typedef std::map<string,int> tag_id_map_t;

// This a 0.1 C++ version of the information in the KML XSD.
// At present it is just the list of elements.  Each element has a name,
// libkml-specific id, and type info (simple vs complex).
class Xsd {
 public:
  static Xsd* GetSchema();

  // Essentially the API to the global <element>'s
  int ElementId(const string& name) const;
  XsdType ElementType(int id) const;
  string ElementName(int id) const;

  // Return the id of the given enum string for the given enum element.
  int EnumId(int type_id, string enum_value) const;
  // Return the enum string for the given enum id for the given enum element.
  string EnumValue(int type_id, int enum_id) const;

 private:
  Xsd();
  static Xsd* schema_;

  tag_id_map_t tag_to_id;
  std::map<int,XsdElement> id_to_string;
};

}  // end namespace kmldom

#endif  // KML_XSD_XSD_H__
