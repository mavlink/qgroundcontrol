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

// This file contains the declaration of the internal StyleInliner class.

#ifndef KML_ENGINE_STYLE_INLINER_INTERNAL_H__
#define KML_ENGINE_STYLE_INLINER_INTERNAL_H__

#include "kml/base/string_util.h"
#include "kml/dom.h"
#include "kml/engine/engine_types.h"

namespace kmlengine {

// This class inlines _most_ shared StyleSelectors into each Feature.
// NOTE: Direct use of this class in production code is not recommended.
// Use the InlineStyles function declared above.
class StyleInliner : public kmldom::ParserObserver {
 public:
  StyleInliner();

  virtual ~StyleInliner() {}

  // ParserObserver::NewElement()
  virtual bool NewElement(const kmldom::ElementPtr& element);

  // Like AsFeature(), but not if the feature is a <Document>.
  static kmldom::FeaturePtr AsNonDocumentFeature(
      const kmldom::ElementPtr& element);

  // ParserObserver::EndElement()
  virtual bool EndElement(const kmldom::ElementPtr& parent,
                          const kmldom::ElementPtr& child);

  // ParserObserver::AddChild()
  virtual bool AddChild(const kmldom::ElementPtr& parent,
                        const kmldom::ElementPtr& child);

  // Mostly for debugging, but no reason to hide these.
  const kmldom::DocumentPtr& get_document() const {
    return document_;
  }

  bool in_update() const {
    return in_update_;
  }

  // All shared styles are added to this map _instead_ of their <Document>
  // parent.  Any duplicate id's are handled in a "last one wins" fashion.
  const kmlengine::SharedStyleMap& get_shared_styles() const {
    return shared_styles_;
  }

 private:
  kmlengine::SharedStyleMap shared_styles_;
  kmldom::DocumentPtr document_;
  bool in_update_;
};

}  // end namespace kmlengine

#endif  // KML_ENGINE_STYLE_INLINER_INTERNAL_H__
