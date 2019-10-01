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

// This file contains the declaration of the HttpClient API.
// TODO: decide if this should really attempt to be a platform agnostic
// authenticated http api or just punt and call it the GoogleHttpClient

#ifndef KML_CONVENIENCE_HTTP_CLIENT_H_
#define KML_CONVENIENCE_HTTP_CLIENT_H_

#include <vector>
#include <memory>
#include "kml/base/net_cache.h"

namespace kmlconvenience {

// TODO: push these to kml/base/string_util.h
typedef std::pair<string, string> StringPair;
typedef std::vector<StringPair> StringPairVector;

// RFC 2616, Section 5.1.1 Method: "GET", "POST", "PUT", "DELETE".
enum HttpMethodEnum {
  HTTP_NONE = 0,
  HTTP_OPTIONS,
  HTTP_GET,
  HTTP_HEAD,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_TRACE,
  HTTP_CONNECT
};

// This class declares an HTTP client interface.  There is nothing directly
// specific to any of KML, Google Maps Data API or the Google Data APIs in
// this _interface_, however the default implementation of the virtual methods
// is specific to Google's Data API ClientLogin.  The typical intended usage
// is to derive a class from this in which the SendRequest() implementation
// performs I/O.  The key purpose of this class is to hold authorization and
// other header state used for a given "session".
class HttpClient : kmlbase::NetFetcher {
 public:
  // The application_name is used in the HTTP User-Agent.
  HttpClient(const string &application_name);

  virtual ~HttpClient() {}

  // These virtuals are the core of the HttpClient interface.

  // The default implementation is as per Google's ClientLogin:
  // http://code.google.com/apis/gdata/auth.html#ClientLogin
  // Derived classes typically are not expected to implement this.  This is
  // virtual primarily for testing.
  virtual bool Login(const string& service_name,
                     const string& email, const string& password);

  // Adds the given field name and value to the set of headers used in every
  // request.  This is a simple append.  No provision is made for overwriting
  // a header field of the same name.
  void AddHeader(const string& field_name,
                 const string& field_value);

  // All I/O goes through this method.  See HttpMethodEnum for valid
  // http_method values.  See RFC 2616, Section 5.1.2 Request-URI for
  // request_uri.  See Section 5.3 for request_headers.  Any of
  // request_headers, data and response may be NULL.  The return value is with
  // regards to the I/O operation itself and is not a reflection of the HTTP
  // response code which is left to the caller to parse out of the response.
  // The response is the "raw" HTTP response and includes all headers.
  // The default implementation performs no I/O and sets the response to the
  // request constructed from the arguments
  virtual bool SendRequest(HttpMethodEnum http_method,
                           const string& request_uri,
                           const StringPairVector* request_headers,
                           const string* post_data,
                           string* response) const;

  // kmlbase::NetFetcher::FetchUrl()
  // The HttpClient implementation of this sends all fetches to SendRequest.
  virtual bool FetchUrl(const string& url, string* data) const;

  // The following static methods are for the convenience of managing headers.

  // This method appends each string pair in src to the end of dest.  If dest
  // is NULL this is a nop.
  static void AppendHeaders(const StringPairVector& src,
                            StringPairVector* dest);

  // If the given headers have a field of the given name return true.  If
  // an output field_value string is supplied the value is saved there.
  static bool FindHeader(const string& field_name,
                         const StringPairVector& headers,
                         string* field_value);

  // This returns the given name-value pair formatted properly for use in an
  // HTTP request.
  static string FormatHeader(const StringPair& header);

  // RFC 2616, Section 4.2.  No formatting such as ':', ' ' or '\n' should
  // appear in the field_name or field_value.  This function appends the
  // given field_name and field_value to the given headers vector.  If the
  // headers vector is NULL this function does nothing.
  static void PushHeader(const string& field_name,
                         const string& field_value,
                         StringPairVector* headers);

  // The following methods return internal information about the state of
  // this HttpClient.  This is primarily for debugging.

  // This returns the internal state of the authorization token.  This will be
  // empty unless Login() was called successfully.
  const string& get_auth_token() const {
    return auth_token_;
  }

  // This returns the internal state of the request headers to be used with
  // each SendRequest.
  const StringPairVector& get_headers() const {
    return headers_;
  }

 private:
  // The key reason for this class is to hold the authorization token from
  // the Login() for use in subsequent SendRequest()'s.
  string auth_token_;
  const string service_name_;
  const string application_name_;
  StringPairVector headers_;
};

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_HTTP_CLIENT_H_
