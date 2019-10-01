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

// This file contains the declaration of the internal StyleMerger class.
// This class is not recommended for use in application code.  Use the
// StyleResolver API instead.

#ifndef KML_ENGINE_STYLE_MERGER_H__
#define KML_ENGINE_STYLE_MERGER_H__

#include "kml/dom.h"
#include "kml/engine/engine_types.h"
#include "kml/engine/kml_file.h"

namespace kmlengine {

class KmlCache;

// This class computes a resolved style for a Feature in a KML file.
// Usage is as follows:
//  // Create a KmlFileNetCache to fetch and parse a KML file:
//  KmlFileNetCache kml_file_net_cache(&your_net_fetcher, max_cache_items);
//  KmlFilePtr = kml_file_net_cache.Fetch(kml_url);
//  // (Parse into a KmlFile makes use of its GetSharedStyleById())
//  // Create a style merger instance for the given style state.
//  StyleMerger* style_merger =
//      StyleMerger::CreateFromKmlFile(kmlfile,
//                                     STYLESTATE_NORMAL|STYLESTATE_HIGHLIGHT);
//  // Merge in the Feature's styleUrl and StyleSelector (both may be empty).
//  style_merger->MergeStyle(styleurl, styleselector);
//  // MergeStyle() recurses down the styleurl as necessary.
//  // The caller retrieves the shared style:
//  StylePtr style = style_merger->ResolvedStyle()
//  // The style itself is non-null, but only those SubStyles with values
//  // found in the resolution process are set.
//  The methods of the StyleResolver is the preferred API to use in
//  application code.
class StyleMerger {
 public:
  // A StyleMerger needs a SharedStyleMap and a style state.  If both a
  // KmlCache and base_url are given then StyleMerger performs networked
  // style resolution to the extent the NetFetcher configured for the KmlCache
  // provides for remote fetching.  If kml_cache is NULL and base_url is empty
  // or if the styleurl does not reference a fetchable address or if the
  // NetFetcher for the supplied KmlCache does not provide access to this URL
  // the given styleurl is effectively (and quietly) ignored.  This constructor
  // defaults the maximum level of styleUrls followed to the KML Engine
  // internal maximum: see engine_constants.h -- as such styleUrl "loops" are
  // detected.
  StyleMerger(const SharedStyleMap& shared_style_map, KmlCache* kml_cache,
              const string& base_url, kmldom::StyleStateEnum style_state);

  // This constructor permits an arbirary styleUrl nesting level to be set.
  StyleMerger(const SharedStyleMap& shared_style_map, KmlCache* kml_cache,
              const string& base_url, kmldom::StyleStateEnum style_state,
              unsigned int max_nested_styleurls);

  // This is a convenience method to create a StyleMerger from a KmlFile.
  static StyleMerger* CreateFromKmlFile(const KmlFile& kml_file,
                                        kmldom::StyleStateEnum style_state);

  // This method is guaranteed to return non-NULL, however the resolved <Style>
  // itself may be devoid of child elements which simply means the style is
  // full default.
  const kmldom::StylePtr& GetResolvedStyle() const {
    return resolved_style_;
  }

  // Both Feature and Pair have a styleUrl and/or StyleSelector.
  void MergeStyle(const string& styleurl,
                  const kmldom::StyleSelectorPtr& styleselector);

  // Merge in the StyleSelector this styleurl references.  Remote fetches are
  // performed through the KmlFileNetCache if one is supplied otherwise
  // remote fetches are quietly ignored.  An empty styleurl is quietly ignored.
  // This returns immediately and has no action if the styleUrl nesting depth
  // is < 0; this facilitates styleUrl loop detection.
  void MergeStyleUrl(const string& styleurl);

  // Merge in the given StyleMap's Pair's whose key's match the style_state_.
  void MergeStyleMap(const kmldom::StyleMapPtr& stylemap);

  // Merge in the given StyleSelector.
  void MergeStyleSelector(const kmldom::StyleSelectorPtr& styleselector);

  // Return the current styleUrl nesting depth.  If this is < 0 no further
  // styleUrl references are followed.  The resolved style is still essentially
  // valid, but it's up to the user of this class to decide if that's an error.
  int get_nesting_depth() const {
    return nesting_depth_;
  }

 private:
  const SharedStyleMap& shared_style_map_;
  KmlCache* kml_cache_;
  string base_url_;
  const kmldom::StyleStateEnum style_state_;
  kmldom::StylePtr resolved_style_;
  int nesting_depth_;
};

}  // endnamespace kmlengine

#endif  // KML_ENGINE_STYLE_MERGER_H__
