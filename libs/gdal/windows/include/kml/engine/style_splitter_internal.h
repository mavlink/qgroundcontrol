// Copyright 2009, Google Inc. All rights reserved.
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

// This file contains the declaration of the internal StyleSplitter class.
// Do not use this class in application code.  Applications should use the
// SplitStyles() function.

#ifndef KML_ENGINE_STYLE_SPLITTER_INTERNAL_H__
#define KML_ENGINE_STYLE_SPLITTER_INTERNAL_H__

#include <map>
#include "kml/base/string_util.h"
#include "kml/dom.h"
#include "kml/dom/parser_observer.h"
#include "kml/engine/engine_types.h"
#include "kml/engine/merge.h"

namespace kmlengine {

// This class splits _most_ inline StyleSelectors to shared style
// NOTE: Direct use of this class in production code is not recommended.
// Use the SplitStyles function declared above.
class StyleSplitter : public kmldom::ParserObserver {
 public:
  // A SharedStyleMap must be supplied.
  StyleSplitter(kmlengine::SharedStyleMap* shared_style_map)
    : shared_style_map_(shared_style_map),
      id_counter_(0),
      in_update_(false) {}

  virtual ~StyleSplitter() {}

  // ParserObserver::NewElement()
  virtual bool NewElement(const kmldom::ElementPtr& element);

  // Like AsFeature(), but not if the feature is a <Document>.
  static kmldom::FeaturePtr AsNonDocumentFeature(
      const kmldom::ElementPtr& element);

  // A convenience routine to create a <Style> or <StyleMap>.
  static kmldom::StyleSelectorPtr CreateStyleSelector(
      kmldom::KmlDomType type_id);

  // The default implementation simply uses the internal sequential id counter
  // as the xml id for the shared style.  A derived class can override this
  // method and use its own naming scheme, however if the id created is not
  // unique the given style will not be split from the feature.
  virtual string CreateUniqueId(const SharedStyleMap& shared_style_map,
                                     unsigned int id_counter) {
    // xml:id cannot begin with a digit.
    return string("_")  + kmlbase::ToString(id_counter);
  }

  // ParserObserver::EndElement()
  virtual bool EndElement(const kmldom::ElementPtr& parent,
                          const kmldom::ElementPtr& child);

  // Mostly for debugging, but no reason to hide these.
  unsigned int get_id_counter() const {
    return id_counter_;
  }

  const kmldom::DocumentPtr& get_document() const {
    return document_;
  }

  bool get_in_update() const {
    return in_update_;
  }

 private:
  kmlengine::SharedStyleMap* shared_style_map_;
  unsigned int id_counter_;
  kmldom::DocumentPtr document_;
  bool in_update_;
};

}  // end namespace kmlengine

#endif  // KML_ENGINE_STYLE_SPLITTER_INTERNAL_H__
