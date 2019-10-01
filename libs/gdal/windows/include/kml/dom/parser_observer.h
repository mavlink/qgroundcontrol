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

// This file defines the ParserObserver base class.

#ifndef KML_DOM_PARSER_OBSERVER_H__
#define KML_DOM_PARSER_OBSERVER_H__

#include "kml/dom/kml_ptr.h"

namespace kmldom {

// This class should be derived from to provide SAX-style callbacks during
// the DOM parse.  The derived class can implement one or both of
// EndElement() and AddChild().  EndElement() is called after the Element
// has been constructed.  AddChild() is called after the given child has
// been added to the given parent.  If the derived class returns false
// from either EndElement() or AddChild() the parse is immediately terminated.
class ParserObserver {
 public:
  virtual ~ParserObserver() {  // Silence compiler warnings.
  }

  // Called after this element is created by the parser.
  virtual bool NewElement(const ElementPtr& element) {
    return true;  // Default implementation is a NOP: parse continues.
  }

  // Called after child is fully constructed before it is added to the parent.
  // A derived class can return false to inhibit adding the child to the parent.
  // Returning true permits the parser to add this child to the parent.
  virtual bool EndElement(const ElementPtr& parent, const ElementPtr& child) {
    return true;
  }

  // Called after the given child has been set to the given parent.
  virtual bool AddChild(const ElementPtr& parent, const ElementPtr& child) {
    return true;  // Default implementation is a NOP: parse continues.
  }
};

typedef std::vector<ParserObserver*> parser_observer_vector_t;

}  // end namespace kmldom

#endif  // KML_DOM_PARSER_OBSERVER_H__
