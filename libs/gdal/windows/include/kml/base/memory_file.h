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

#ifndef KML_BASE_MEMORY_FILE_H__
#define KML_BASE_MEMORY_FILE_H__

#include "boost/intrusive_ptr.hpp"
#include "kml/base/referent.h"
#include "kml/base/util.h"

namespace kmlbase {

// This NetCache CacheItem is a utility class used to represent file contents
// cached in memory.  The intended usage is as follows:
//   NetCache<YourNetFetcher, MemoryFile> memory_file_net_cache;
//   MemoryFile memory_file = memory_file_net_cache.Fetch(url);
//   const string& file_content = memory_file.get_content();
class MemoryFile : public Referent {
 public:
   static MemoryFile* CreateFromString(const string& data) {
     return new MemoryFile(data);
   }

   const string& get_content() const {
     return content_;
   }

 private:
  MemoryFile(const string& content) : content_(content) {}
  string content_;
};

typedef boost::intrusive_ptr<MemoryFile> MemoryFilePtr;

}  // end namespace kmlbase

#endif  // KML_BASE_MEMORY_FILE_H__
