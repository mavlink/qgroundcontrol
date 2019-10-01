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

// This file declares the KmlHandlerNS subclass of KmlHandler.
// This is used internally to the ParseNS() function. See kml_handler.h for
// details.

#ifndef KML_DOM_KML_HANDLER_NS_H__
#define KML_DOM_KML_HANDLER_NS_H__

#include <stack>
#include <string>
#include "kml_handler.h"
#include "kml/base/expat_handler.h"
#include "kml/dom/element.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/parser_observer.h"

namespace kmldom {

class KmlFactory;

// This subclass of KmlHandler is used with Expat's namespace-aware parsing.
class KmlHandlerNS : public KmlHandler {
 public:
  KmlHandlerNS(parser_observer_vector_t& observers);
  ~KmlHandlerNS();

  // ExpatHandler methods.
  virtual void StartElement(const string &name,
                            const kmlbase::StringVector& atts);
  virtual void EndElement(const string &name);
  virtual void CharData(const string &s);
  virtual void StartNamespace(const string &prefix,
                              const string &uri);
  virtual void EndNamespace(const string &prefix);

 private:
  // TODO: A map of namespace URIs to their prefixes found during the parse.
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(KmlHandlerNS);
};

} // end namespace kmldom

#endif  // KML_DOM_KML_HANDLER_NS_H__
