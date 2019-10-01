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

// This file contains some utility classes for use with NetCache unit tests.

#ifndef KML_BASE_NET_CACHE_TEST_UTIL_H__
#define KML_BASE_NET_CACHE_TEST_UTIL_H__

#include "boost/scoped_ptr.hpp"
#include "kml/base/file.h"
#include "kml/base/net_cache.h"
#include "kml/base/uri_parser.h"

// The following define is a convenience for testing inside Google.
#ifdef GOOGLE_INTERNAL
#include "kml/base/google_internal_test.h"
#endif

#ifndef DATADIR
#error *** DATADIR must be defined! ***
#endif

namespace kmlbase {

// This NetFetcher maps the URI path to the testdata directory.  The scheme
// and host are ignored.
class TestDataNetFetcher : public NetFetcher {
 public:
  bool FetchUrl(const string& url, string* data) const {
    boost::scoped_ptr<UriParser> uri_parser(
        UriParser::CreateFromParse(url.c_str()));
    string path;
    // If the URI parse succeeds, and a data buffer was provided, and the
    // URI has a path, and the file system read succeeds return true.
    return uri_parser.get() && data && uri_parser->GetPath(&path) &&
           kmlbase::File::ReadFileToString(kmlbase::File::JoinPaths(DATADIR,
                                                                    path),
                                           data);
  }
};

}  // end namespace kmlbase

#endif  // KML_BASE_NET_CACHE_TEST_UTIL_H__
