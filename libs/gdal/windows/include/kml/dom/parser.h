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

// This file contains the internal Parser class.

#ifndef KML_DOM_PARSER_H__
#define KML_DOM_PARSER_H__

#include <vector>
#include "kml/dom/kml_ptr.h"
#include "kml/dom/parser_observer.h"
#include "kml/base/util.h"

namespace kmldom {

// The internal Parser class implements the public Parse API.
// CDATA tags are dropped (by expat) upon parse and internally we carry
// around the resultant representation. There are thus no methods within
// the KML DOM to set/get/query for the presence of CDATA. The serializer
// scans the markup and conditionally wraps entities with CDATA. For files
// with multiple CDATA wrappers in a single element's character data, this
// will result in a single CDATA wrapper.
//
// Intended usage:
//   Parser parser;
//   parser.AddObserver(...);
//   parser.AddObserver(...);
//   ...
//   string errors;
//   ElementPtr root = parser.Parse(kml, &errors);
class Parser {
 public:
  Parser() {}
  // This method calls the parser with the given KML string.  If there are
  // any errors NULL is returned and if error's is non-NULL a human readable
  // diagnostic is stored there.  If there are no parse errors the root
  // element is returned.  Note that any ParseObserver can terminate the parse.
  ElementPtr Parse(const string& kml, string *errors);

  // As Parse(), but invokes the underlying XML parser's namespace-aware mode.
  ElementPtr ParseNS(const string& kml, string *errors);

  // As Parse(), but invokes the underlying XML parser's namespace-aware mode
  // with special recognition of the Atom namespace.  See kml_funcs.h.
  ElementPtr ParseAtom(const string& atom, string *errors);

  // This method registers the given ParserObserver-based class.  Each
  // NewElement() and AddChild() method is called in the order added.
  void AddObserver(ParserObserver* parser_observer);
 private:
  parser_observer_vector_t observers_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Parser);
};

}  // end namespace kmldom

#endif  // KML_DOM_PARSER_H__
