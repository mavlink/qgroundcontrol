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

// This file contains the declaration of the GetElementsByType() and the
// internal ElementFinder class.

#ifndef KML_ENGINE_FIND_H__
#define KML_ENGINE_FIND_H__

#include <stack>
#include <string>
#include "kml/dom.h"
#include "kml/engine/engine_types.h"

namespace kmlengine {

// Starting at the hierarchy rooted at element this finds all complex elements
// of the given type and appends an ElementPtr to each in the given array.
// The element is not cloned.  The array is simple a list into the DOM.
// Since ElementPtr is reference counted it is safe to release any references
// to any parent of any element in the array.
// TODO: decide const vs non-const semantics: modifications to an Element
// in one place might wreak havoc on usage elsewhere.
void GetElementsById(const kmldom::ElementPtr& element,
                     kmldom::KmlDomType type_id,
                     ElementVector* element_vector);

// Get all children of the given element.  If recurse is true find all children
// hierarchically.  If element_vector is non-NULL all children found are
// appended in depth-first order.  The return value is the number of children
// encountered.
int GetChildElements(const kmldom::ElementPtr& element, bool recurse,
                     ElementVector* element_vector);

}  // end namespace kmlengine

#endif  // KML_ENGINE_FIND_H__
