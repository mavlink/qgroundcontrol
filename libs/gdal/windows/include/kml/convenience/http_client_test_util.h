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

#ifndef KML_CONVENIENCE_HTTP_CLIENT_TEST_UTIL_H_
#define KML_CONVENIENCE_HTTP_CLIENT_TEST_UTIL_H_

#include <vector>
#include "kml/base/file.h"
#include "kml/base/net_cache_test_util.h"
#include "kml/convenience/http_client.h"

namespace kmlconvenience {

// This HttpClient always "fetches" the given file.
class OneFileHttpClient : public HttpClient {
 public:
  OneFileHttpClient(const string& fetch_me)
    : HttpClient("OneFileHttpClient"),
      fetch_me_(fetch_me) {
  }

  virtual bool SendRequest(HttpMethodEnum http_method,
                           const string& request_uri,
                           const StringPairVector* request_headers,
                           const string* post_data,
                           string* response) const {
    return kmlbase::File::ReadFileToString(fetch_me_.c_str(), response);
  }

 private:
  const string fetch_me_;
};

// This HttpClient logs each request to the supplied vector.
struct HttpRequest {
  HttpMethodEnum http_method_;
  string request_uri_;
  StringPairVector request_headers_;
  string post_data_;
};
typedef std::vector<HttpRequest> HttpRequestVector;

class LoggingHttpClient : public HttpClient {
 public:
  LoggingHttpClient(HttpRequestVector* request_log)
    : HttpClient("LoggingHttpClient"),
      request_log_(request_log) {
  }

  virtual bool SendRequest(HttpMethodEnum http_method,
                           const string& request_uri,
                           const StringPairVector* request_headers,
                           const string* post_data,
                           string* response) const {
    HttpRequest http_request;
    http_request.http_method_ = http_method;
    http_request.request_uri_ = request_uri;
    if (request_headers) {
      http_request.request_headers_ = *request_headers;
    }
    if (post_data) {
      http_request.post_data_ = *post_data;
    }
    request_log_->push_back(http_request);
    return true;
  }

 private:
  HttpRequestVector* request_log_;
};

// This HttpClient fetches the URI using kmlbase::TestDataNetFetcher which
// reads the testdata file named by the path portion of the URI.
class TestDataHttpClient : public HttpClient {
 public:
  TestDataHttpClient()
    : HttpClient("TestDataHttpClient") {
  }
  virtual bool SendRequest(HttpMethodEnum http_method,
                           const string& request_uri,
                           const StringPairVector* request_headers,
                           const string* post_data,
                           string* response) const {
    return test_data_net_fetcher_.FetchUrl(request_uri, response);
  }
 private:
  kmlbase::TestDataNetFetcher test_data_net_fetcher_;
};

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_HTTP_CLIENT_TEST_UTIL_H_
