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

#ifndef KML_ENGINE_KML_CACHE_H__
#define KML_ENGINE_KML_CACHE_H__

#include "kml/base/net_cache.h"
#include "boost/scoped_ptr.hpp"
#include "kml/engine/kml_file.h"
#include "kml/engine/kmz_cache.h"

namespace kmlengine {

// A cache of KmlFile's (parse of a KML file of a given URL).
typedef kmlbase::NetCache<KmlFile> KmlFileNetCache;

// This class is the main public API for networked KML.  Overall usage is as
// follows:
//   class YourNetFetcher : public kmlbase::NetFetcher {
//     // see kmlbase::NetCache
//   };
//   YourNetFetcher your_net_fetcher;
//   KmlCache kml_cache(&your_net_fetcher, cache_size);
//   KmlFilePtr k0 = kml_cache.FetchKmlAbsolute("http://host.com/file.kml");
//   KmlFilePtr k1 = kml_cache.FetchKmlAbsolute("http://host.com/file.kmz");
//   KmlFilePtr k2 =
//        kml_cache.FetchKmlAbsolute("http://host.com/file.kmz/foo.kml");
//   KmlFilePtr k3 =
//        kml_cache.FetchKmlRelative("http://host.com/file.kmz/doc.kml",
//                                   "link.kml");
//   string data;
//   bool status = kml_cache.FetchDataRelative("http://host.com/overlay.kml",
//                                             "image.jpg", &data);
//   bool status =
//       kml_cache.FetchDataRelative("http://host.com/file.kmz/doc.kml"
//                                   "image.jpg", &data);
// As the "cache" name suggests subsequent fetches for a given URL will
// potentially hit the cache.
class KmlCache {
 public:
  KmlCache(kmlbase::NetFetcher* net_fetcher, size_t max_size);

  // Any caller expecting to fetch and parse KML data should use this method.
  // Use this with the raw content of a NetworkLink/Link/href, styleUrl, or
  // schemaUrl.  A given parse of a local or remote StyleSelector or Schema
  // referenced by a styleUrl/schemaUrl is thus cached.  The returned KmlFile
  // is marked with a pointer back to this cache such that other internal
  // KML Engine algorithms can fetch (and cache) shared styles and schemas.
  // The base_url is typically that of the file containing the target_href.
  KmlFilePtr FetchKmlRelative(const string& base_url,
                              const string& target_href);

  // This method is used to fetch a remote KML or KMZ file with an absolute URL.
  // If the fetch or parse fails NULL is returned.
  KmlFilePtr FetchKmlAbsolute(const string& kml_url);

  // Any caller expecting to fetch data which _may_ be within a KMZ should use
  // this method.  If the data is within a remote KMZ file that KMZ file is
  // first fetched and cached such that subsequent access to this or other files
  // within that KMZ file are out of the locally cached KMZ file.  Such content
  // includes Model/Link/href (COLLADA geometry) and images for icons, overlays
  // or model textures.  The target_href here typically is the content of an
  // Overlay Icon's href, or Model's Link href.  The base_url is typically that
  // of the file containing the target_href.
  bool FetchDataRelative(const string& base_url,
                         const string& target_href,
                         string* content);

 private:
  boost::scoped_ptr<KmzCache> kmz_file_cache_;
  boost::scoped_ptr<KmlFileNetCache> kml_file_cache_;
};

}  // end namespace kmlengine

#endif  // KML_ENGINE_KML_CACHE_H__
