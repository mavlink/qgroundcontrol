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

// This file contains the NetCache class template and NetFetcher base class.

#ifndef KML_BASE_NET_CACHE_H__
#define KML_BASE_NET_CACHE_H__

#include <map>
#include "kml/base/util.h"
#include "boost/intrusive_ptr.hpp"

namespace kmlbase {

// A CacheItem is derived from Referent and has a CreateFromString:
// class SomeCacheItem : public Referent {
//  public:
//   static SomeCacheItem* CreateFromString(const string& data);
// };

// This is the default NetFetcher.  It represents the empty network which
// simply returns false for all URLs.  This is provided for non-networked
// libkml usage and effectively stubs out network access.  This is useful in
// the several places in KML where failed network fetch is quietly ignored.
// Application code should derive from NetFetcher and implement FetchUrl
// to perform (synchronous) network fetching as desired.  All external I/O
// from within NetCache is called out to the application code in this manner.
class NetFetcher {
 public:
  virtual ~NetFetcher() {}
  virtual bool FetchUrl(const string& url, string* data) const {
    return false;
  }
};

// This class template provides a generic memory cache facility parameterized
// by the CacheItem.  Typical usages is as follows:
// Create a "CacheItem" class as described above (one that implements a
// CreateFromString with the signature given above):
//   class MyCacheItem : public kmlbase::Referent {
//    public:
//     static MyCacheItem* CreateFromString(const string& data) {
//       MyCacheItem* my_cache_item = new MyCacheItem;
//       // whatever else your CacheItem does with an input data buffer
//       return my_cache_item;
//     }
//     // other methods if you have them
//   };
//   typedef boost::intrusive_ptr<MyCacheItem> MyCacheItemPtr;
//
// Create a NetCache for that CacheItem:
//   NetCache<MyCacheItem> net_cache_of_my_cache_items;
//
// Create a NetFetcher class by inheriting from NetFetcher as described above:
//   class MyNetFetcher : kmlbase::NetFetcher {
//    public:
//     virtual bool FetchUrl(const string& url, string* data) const {
//       // do however it is you want to fetch the url, save the content to data
//       // Note that true means that this IS the data for this URL (not
//       // a 404 page... _your_ code must detect higher level protocol issues).
//       return true;  // or false is a fetch on that url failed
//     }
//    // other methods if you have them
//   };
//
// Your application code now fetches and creates CacheItems for a given URL
// by calling Fetch:
//   MyCacheItemPtr a = net_cache_of_my_cache_items.Fetch(some-url);
//   MyCacheItemPtr b = net_cache_of_my_cache_items.Fetch(some-other-url);
// When the NetCache goes out of scope all cached CacheItems are deleted,
// however use of boost::intrusive_ptr does permit any code to hold a pointer
// to an item originally from cache beyond the cache's lifetime.
// NOTE: This class is NOT thread safe!
template<class CacheItem>
class NetCache {
 public:
  typedef boost::intrusive_ptr<CacheItem> CacheItemPtr;
  typedef std::pair<CacheItemPtr, uint64_t> CacheEntry;
  typedef std::map<string, CacheEntry> CacheMap;

  // Construct the NetCache with the given NetFetcher-derived class and
  // with the given limit on number of items to cache.  This size is entirely
  // application specific, but it should noted that CacheItems _may_ hold
  // file descriptors open so platform limits may limit max_size.  Typical
  // sizes are expected to be in the 10s to 100s of items.
  NetCache(NetFetcher* net_fetcher, size_t max_size)
      : max_size_(max_size),
        cache_count_(0),
        net_fetcher_(net_fetcher) {}

  // This is the main public method in NetCache.  If the NetFetcher FetchUrl
  // returns true for this url the data fetched is passed to CreateFromString
  // on the CacheItem to create a CacheItem from this data.  This CacheItem
  // is saved to the cache.  If the cache has reached its limit as set in
  // the constructor the oldest entry is discarded from the cache.  If the
  // CacheItem for this URL is in the cache it is simply returned.
  CacheItemPtr Fetch(const string& url) {
    // If an item is cached for this URL return it and we're done.
    if (CacheItemPtr item = LookUp(url)) {
      return item;
    }
    // Not found in cache: go fetch.
    string data;
    // NetFetcher knows only about "get me the data at this URL".
    if (!net_fetcher_->FetchUrl(url, &data)) {
      return NULL;  // Fetch failed, no such URL.
    }
    // Fetch succeeded: create a CacheItem from the data.
    CacheItemPtr item = CacheItem::CreateFromString(data);
    if (!Save(url, item)) {  // This is basically an internal error.
      return NULL;
    }
    return item;
  }

  // This returns the CacheItem in the cache for the given url if it exists.
  // If nothing is cached for this url then NULL is returned.
  // In typical usage this method is not used by application code, but it is
  // well behaved as described.
  const CacheItemPtr LookUp(const string& url) const {
    typename CacheMap::const_iterator iter = cache_map_.find(url);
    if (iter == cache_map_.end()) {
      return NULL;
    }
    // iter->first is key, second is val and val is KmzCacheEntry pair whose
    // first is KmlFilePtr (second is creation time of cache entry).
    return iter->second.first;
  }

  // This stores the given CacheItem to the cache for the given url.
  // This failes of a CacheItem for this url exists (use Delete first).
  // If the cache is at capacity this also first forces the removall
  // of the oldest item in the cache.  Application code should not typically
  // use this directly: use Fetch().
  bool Save(const string& url, const CacheItemPtr& cache_item) {
    const CacheItemPtr exists = LookUp(url);
    if (exists) {
      return false;
    }
    if (cache_map_.size() == max_size_) {
      RemoveOldest();
    }
    // It is not expected cache_count_ ever roll over.  See net_cache_test.cc
    // for some timing tests and results.
    CacheEntry cache_entry = std::make_pair(cache_item, cache_count_++);
    cache_map_[url] = cache_entry;
    return true;
  }

  // If a CacheItem exists for this url it is deleted and true is returned.
  // If no CacheItem exists for this url false is returned.  Application code
  // should generally have no need to use this directly.
  bool Delete(const string& url) {
    const CacheItemPtr cache_item = LookUp(url);
    if (cache_item) {
      cache_map_.erase(url);
      return true;
    }
    return false;
  }

  // This removes the oldest entry in the cache.  Application code should
  // generally not need to use this directly.
  bool RemoveOldest() {
    if (cache_map_.empty()) {
      return false;
    }
    // Find the entry with the smallest time.
    typename CacheMap::iterator iter = cache_map_.begin();
    typename CacheMap::iterator oldest = iter;
    for (;iter != cache_map_.end(); ++iter) {
      // STL map iter is a pair<key,val> with val CacheItem which is a pair
      // whose second is the timestamp.
      if (iter->second.second < oldest->second.second) {
        oldest = iter;
      }
    }
    cache_map_.erase(oldest);
    return true;
  }

  // This returns the number of items presently in the cache.
  size_t Size() const {
    return cache_map_.size();
  }

 private:
  const size_t max_size_;
  CacheMap cache_map_;
  uint64_t cache_count_;
  const NetFetcher* net_fetcher_;
};

}  // end namespace kmlbase

#endif  // KML_BASE_NET_CACHE_H__
