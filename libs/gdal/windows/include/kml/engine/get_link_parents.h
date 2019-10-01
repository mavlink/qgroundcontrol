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

// This file contains the declaration of the GetLinkParents() function and the
// definition of the GetLinkParentsParserObserver.

#ifndef KML_ENGINE_GET_LINK_PARENTS_H__
#define KML_ENGINE_GET_LINK_PARENTS_H__

#include <vector>
#include "kml/dom.h"
#include "kml/engine/engine_types.h"

namespace kmlengine {

// Is this element the parent of a Link?
bool IsLinkParent(const kmldom::ElementPtr& element);

// Is this element the parent of an Icon?
bool IsIconParent(const kmldom::ElementPtr& element);

// This ParserObserver looks for all elements that have a link child.
class GetLinkParentsParserObserver : public kmldom::ParserObserver {
 public:
  GetLinkParentsParserObserver(ElementVector* link_parent_vector)
      : link_parent_vector_(link_parent_vector) {}

  virtual ~GetLinkParentsParserObserver() {}

  virtual bool NewElement(const kmldom::ElementPtr& element) {
    if (IsLinkParent(element) || IsIconParent(element)) {
      link_parent_vector_->push_back(element);
    }
    return true;
  }

 private:
  ElementVector* link_parent_vector_;
};

// This function appends all Elements with link children to the supplied
// vector.  This returns false if a NULL vector pointer is supplied or if the
// parse of the kml fails.  See the IsLinkParent() and IsIconParent()
// functions for the definition of "Element with link child".
bool GetLinkParents(const string& kml, ElementVector* link_parent_vector);

}  // end namespace kmlengine

#endif  // KML_ENGINE_GET_LINK_PARENTS_H__
