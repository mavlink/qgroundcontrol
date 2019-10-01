// Copyright 2008, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,//     this list of conditions and the following disclaimer in the documentation//     and/or other materials provided with the distribution.
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

// This file contains the declaration of the "old-schema" parsing functions.

#ifndef KML_ENGINE_PARSE_OLD_SCHEMA_H__
#define KML_ENGINE_PARSE_OLD_SCHEMA_H__

#include "kml/dom.h"
#include "kml/engine/engine_types.h"

namespace kmlengine {

// This is how "old-style" <Schema> worked.  (This is NOT how <Schema> is used
// in OGC KML 2.2.  This function is provided to map old-style <Schema> to
// OGC-compliant KML.
//
// In old-style <Schema> the following construct essentially extended
// <Placemark> to add a <NAME> field of the given type.  All instances of this
// element would then appear as <S_park_boundaries_S>...</S_park_boundaries_S>
// and would generally take the form of a <Placemark>.  Tecnically speaking
// the parent could be anything, but <Placemark> was the only element ever
// extended in this fashion.
//
// <Schema parent="Placemark" name="S_park_boundaries_S">
//   <SimpleField type="wstring" name="NAME"/>
// </Schema>
//
// <S_park_boundaries_S>
//   <name>Arches NP</name>
//   <Polygon>...</Polygon>
//   <NAME>Arches NP</NAME>
// </S_park_boundaries_S>
//
// Assuming the above <Schema> is in the passed map the above sample is parsed
// to the following:
//
// <Placemark>
//   <name>Arches NP</name>
//   <Polygon>...</Polygon>
//   <ExtendedData>
//     <Data name="NAME">Arches NP</Data>
//   </ExtendedData>
//   <NAME>Arches NP</NAME>
// </Placemark>

// If the input_xml is an element whose name is in the SchemaNameMap then
// this returns a converted <Placemark> to the given output buffer.
bool ConvertOldSchema(const string& input_xml,
                      const SchemaNameMap& schema_name_map,
                      string* output_xml);

// This uses ConvertOldSchema() to convert and parse the input.  If anything
// fails NULL is returned and errors are saved to the given error buffer.
kmldom::PlacemarkPtr ParseOldSchema(const string& xml,
                                    const SchemaNameMap& schema_name_map,
                                    string* errors);

}  // end namespace kmlengine

#endif  // KML_ENGINE_PARSE_OLD_SCHEMA_H__
