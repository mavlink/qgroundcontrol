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

// This file contains the declaration of the public Parse and Serialize API
// functions.

#ifndef KML_DOM_KML_FUNCS_H__
#define KML_DOM_KML_FUNCS_H__

#include <ostream>
#include "kml/dom/element.h"
#include "kml/dom/kml_ptr.h"

namespace kmldom {

// Parse the KML in the given memory buffer.  On success this returns an
// Element* to the root of the KML.  On failure 0 is returned and a human
// readable error string is stored to errors if such is supplied.
ElementPtr Parse(const string& xml, string* errors);

// As Parse(), but invokes the underlying XML parser's namespace-aware mode.
ElementPtr ParseNS(const string& xml, string* errors);

// As Parse(), but invokes the underlying XML parser's namespace-aware mode
// such that both prefixed and non-prefixed Atom is recognized as the root.
// Use this to parse "<feed xmlns='http://www.w3.org/2005/Atom'>...", or
// "<atom:feed xmlns:atom='http://www.w3.org/2005/Atom'>...".  The Atom
// namespace MUST be supplied.
ElementPtr ParseAtom(const string& atom, string* errors);

// This is a simplified interface for the benefit of SWIG.
ElementPtr ParseKml(const string& xml);

// This function is the public API for generating "pretty" XML for the KML
// hierarchy rooted at the given Element.  "pretty" is 2 space indent for
// each level of XML depth.
string SerializePretty(const ElementPtr& root);

// This function is the public API for generating "raw" XML for the KML
// hierarchy rooted at the given Element.  "raw" is no indentation white space
// and no newlines.
string SerializeRaw(const ElementPtr& root);

// This function is the public API for emitting the XML of an element
// hierarchy.  The comments for SerializePretty() vs SerializeRaw() describe
// the behavior of the "pretty" flag.  If root or xml are null this method
// does nothing and immediately returns.
void SerializeToOstream(const ElementPtr& root, bool pretty, std::ostream* xml);

// This function is the public API for returning the element's tag name, for
// example "Placemark" for <Placemark> and "NetworkLink" for <NetworkLink>.
// If element is NULL or otherwise invalid an empty string is returned.
string GetElementName(const ElementPtr& element);

}  // end namespace kmldom

#endif  // KML_DOM_KML_FUNCS_H__
