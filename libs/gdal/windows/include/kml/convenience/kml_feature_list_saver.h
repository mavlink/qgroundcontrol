// Copyright 2010, Google Inc. All rights reserved.
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

// This file contains the implementation of the KmlFeatureListSaver class.

#ifndef KML_CONVENIENCE_KML_FEATURE_LIST_SAVER_H__
#define KML_CONVENIENCE_KML_FEATURE_LIST_SAVER_H__

#include "kml/convenience/feature_list.h"
#include "kml/dom.h"
#include "kml/engine/engine_types.h"

namespace kmlconvenience {

// This ParserObserver saves the non-Container Features in the input KML
// to the given FeatureList and the shared style selectors to the given
// SharedStyleMap if one is supplied.  If a style_base is supplied any Feature
// with a in-file relative reference is saved to the FeatureList with the
// given string as the base.  The SharedStyleMap and style_base are optional.
// Example usage:
//  FeatureList feature_list;
//  SharedStyleMap shared_style_map;
//  KmlFeatureListSaver kml_saver(&feature_list, &shared_style_map, "s.kml");
//  Parser parser;
//  parser.AddObserver(&kml_saver);
//  string errors;
//  parser.Parse(kml, &errors);
class KmlFeatureListSaver : public kmldom::ParserObserver {
 public:
  KmlFeatureListSaver(FeatureList* feature_list,
                      kmlengine::SharedStyleMap* shared_style_map,
                      const char* style_base)
    : feature_list_(feature_list),
      shared_style_map_(shared_style_map),
      in_update_(false) {
    if (style_base) {
      style_base_ = string(style_base);
    }
  }

  virtual bool StartElement(const kmldom::ElementPtr& element) {
    if (element->Type() == kmldom::Type_Update) {
      in_update_ = true;
    }
    return true;
  }

  virtual bool EndElement(const kmldom::ElementPtr& parent,
                          const kmldom::ElementPtr& child) {
    if (child->Type() == kmldom::Type_Update) {  // </Update>
      in_update_ = false;
      return false;
    }
    if (in_update_) {
      return true;
    }
    if (child->IsA(kmldom::Type_Feature) &&
        !child->IsA(kmldom::Type_Container)) {
      kmldom::FeaturePtr feature = kmldom::AsFeature(child);
      if (!style_base_.empty() && feature->has_styleurl()) {
        const string& styleurl = feature->get_styleurl();
        if (styleurl.size() > 2 && styleurl[0] == '#') {
          feature->set_styleurl(style_base_ + styleurl);
        }
      }
      feature_list_->PushBack(feature);
      return false;
    }
    if (shared_style_map_ && child->IsA(kmldom::Type_StyleSelector) &&
        parent->IsA(kmldom::Type_Document)) {
      const kmldom::StyleSelectorPtr ss = kmldom::AsStyleSelector(child);
      if (ss->has_id()) {
        (*shared_style_map_)[ss->get_id()] = ss;
        return false;
      }
    }
    return true;
  }

 private:
  FeatureList* feature_list_;
  kmlengine::SharedStyleMap* shared_style_map_;
  string style_base_;
  bool in_update_;
};

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_KML_FEATURE_LIST_SAVER_H__
