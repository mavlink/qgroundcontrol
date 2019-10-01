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

// This file contains the internal KmzCache class declaration.

#ifndef KML_ENGINE_KMZ_CACHE_H__
#define KML_ENGINE_KMZ_CACHE_H__

#include <map>
#include "boost/scoped_ptr.hpp"
#include "kml/base/memory_file.h"
#include "kml/base/net_cache.h"
#include "kml/engine/kmz_file.h"

namespace kmlengine {

class KmlUri;

// This class is a cache of network-fetched KmzFile's.  The NetFetcher
// is supplied by the caller to implement application-specific networking.
// See kmlbase::NetCache for more information.
// NOTE: Applications should generally use KmlCache.
class KmzCache : public kmlbase::NetCache<KmzFile> {
  typedef kmlbase::NetCache<kmlbase::MemoryFile> MemoryFileCache;
 public:
  // This creates a KmzCache to hold up to the given number of KmzFiles.
  // This same size is used for an internal cache of MemoryFile's of fetched
  // files which are not KMZ.
  KmzCache(kmlbase::NetFetcher* net_fetcher_, size_t max_size)
    : kmlbase::NetCache<KmzFile>(net_fetcher_, max_size) {
    memory_file_cache_.reset(new MemoryFileCache(net_fetcher_, max_size));
  }

  // This is the main KML Engine internal method to perform a KMZ-aware fetch.
  // KmlUri encodes the fetch base and target.  The data fetched is stored to
  // the content string.  False is returned if kml_uri or content are NULL or
  // if the fetch fails.  If a fetched_url arg is supplied the actual URL
  // fetched is stored there.
  bool DoFetchAndReturnUrl(KmlUri* kml_uri, string* content,
                           string* fetched_url);

  // This wrapper is supplied for backwards compat.
  bool DoFetch(KmlUri* kml_uri, string* content) {
    return DoFetchAndReturnUrl(kml_uri, content, NULL);
  }

  // This is basically an internal helper method to perform a simple lookup
  // of a file within a KMZ.  If the KmlUri describes a KMZ file in the cache
  // and the target is exists within that KMZ then the content is saved to
  // the supplied buffer and true is returned.  If the KmlUri is not KMZ
  // related or if the target is not within the KMZ or if no content buffer
  // is supplied false is returned.
  bool FetchFromCache(KmlUri* kml_uri, string* content) const;

 private:
  boost::scoped_ptr<MemoryFileCache> memory_file_cache_;
};

}  // end namespace kmlengine

#endif  // KML_ENGINE_KMZ_CACHE_H__
