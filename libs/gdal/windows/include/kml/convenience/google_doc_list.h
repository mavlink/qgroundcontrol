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

// This file contains the declaration of the GoogleDocList class.
// Very distantly related to:
// http://code.google.com/p/gdata-cpp-util/source/browse/trunk/gdata/client/doc_list_service.h
// TODO: this interface and implemenation are experimental, expect additions
// and changes.

#ifndef KML_CONVENIENCE_GOOGLE_DOC_LIST_H_
#define KML_CONVENIENCE_GOOGLE_DOC_LIST_H_

#include <string>
#include <vector>
#include "boost/scoped_ptr.hpp"
#include "kml/dom.h"

namespace kmlconvenience {

class HttpClient;

// This class provides an API to the Google DocList API.
// See the Google Documents List Data API v3.0 Protocol Guide.
// http://code.google.com/apis/documents/docs/3.0/developers_guide_protocol.html
// Overall usage:
//   class YourHttpClient : public kmlconvenience::HttpClient {
//    public:
//     virtual bool SendRequest(...) const {
//       ...your network I/O code goes here...
//     };
//   };
//   YourHttpClient* your_http_client = new YourHttpClient;
//   your_http_client->Login("user@gmail.com", "users-password");
//   GoogleDocList* maps_data = GoogleDocList::Create(&your_http_client);
//   string spreadsheets_feed;
//   maps_data->GetMetaFeed(&spreadsheets_feed);
//   kmldom::ElementPtr root = kmldom::ParseAtom(spreadsheets_feed);
//   kmlconvenience::AtomUtil... for common Atom inspection.
class GoogleDocList {
 public:
  // Create a GoogleDocList object.  The HttpClient must already be logged
  // in.  See http_client.h for more information about authentication.
  // The HttpClient is used for all network communication from inside this
  // GoogleDocList instance.  The GoogleDocList object
  // takes ownership of the HttpClient and destroys it when
  // the GoogleDocList object is destroyed.
  static GoogleDocList* Create(HttpClient* http_client);

  ~GoogleDocList();

  static const char* get_service_name();

  static const char* get_metafeed_uri();

  const string& get_scope() const {
    return scope_;
  }

  // This returns the "meta feed" for the authenticated user.  The result is an
  // Atom <feed> containing an <entry> for each spreadsheet.  See:
  // http://code.google.com/apis/spreadsheets/data/3.0/developers_guide_protocol.html#ListingDocList
  bool GetMetaFeedXml(string* atom_feed) const;

  // This calls GetMetaFeedXml and returns the parsed result.
  kmldom::AtomFeedPtr GetMetaFeed() const;

  // http://code.google.com/apis/documents/docs/3.0/developers_guide_protocol.html#UploadingDocs
  bool UploadSpreadsheet(const string& spreadsheet,
                         const string& mimetype,
                         string* atom_entry);

 private:
  // Use static Create().
  GoogleDocList();
  boost::scoped_ptr<HttpClient> http_client_;
  const string scope_;
};

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_GOOGLE_DOC_LIST_H_
