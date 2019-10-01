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

// This file contains the declaration of the internal UriParser class that
// front-ends the third_party/uriparser in a C++ API.  See kml/engine/kml_uri.h
// for the public API to URI handling.

#ifndef KML_BASE_URI_PARSER_H__
#define KML_BASE_URI_PARSER_H__

#include "kml/base/util.h"
#include "boost/scoped_ptr.hpp"

namespace kmlbase {

class UriParserPrivate;

// This class is a memory-safe wrapper to uriparser's UriUriA.
class UriParser {
 public:
  // UriParser is always constructed from one of the following static methods.
  // The main intentended usage of UriParser is within libkml and is restricted
  // to these static methods.

  // This creates a UriParser from a URI in string form.
  static UriParser* CreateFromParse(const char* str);

  // This creates a UriParser representing the resolution of the given
  // relative URI against the given base URI.
  static UriParser* CreateResolvedUri(const char* base, const char* relative);

  // The intended usage is to create a UriParser from a static method.
  UriParser();

  // The destructor must perform uriparser-specific operations to release
  // resources.  It is highly recommdended that a UriParser* be managed
  // with boost::scoped_ptr or equivalent (as is done in CreateResolveUri).
  ~UriParser();

  // This parses the given URI string into the UriParser object and obliterates
  // any previous URI parsed into this object.  If the parse succeeds true is
  // returned, else false is returned.  This method is intended for use mainly
  // with the CreateFromParse() static method.
  bool Parse(const char* str);

  // UriParser (and the underlying uriparser library) does not automatically
  // normalize any URI.  (Normalize resolves the ..'s in a path, for example).
  // This method may be used at any time to normalize the URI.  RFC 3986
  // requires a fetching client to normalize a URI before fetching it.
  bool Normalize();

  // This resolves the URI represented by the UriParser relative against the
  // URI represented by the UriParser base.  This method is intended for use
  // mainly with the CreateResolvedUri() static method.
  bool Resolve(const UriParser& base, const UriParser& relative);

  // This method saves the URI in string form into the given string.  This
  // returns false if a NULL string argument is supplied or on any internal
  // errors in saving to this string.  True is returned on success.
  bool ToString(string* output) const;

  // Converts a URI to its corresponding filename. The implementation
  // is platform-independent and handles either UNIX- or Windows-style path
  // names transparently. Returns false if output is NULL or on any internal
  // error in converting the uri.
  static bool UriToFilename(const string& uri, string* output);

  // Converts a UNIX URI to its corresponding UNIX filename. Returns false if
  // output is NULL or on any internal error in converting the uri.
  // For example, calling this function on "file:///home/libkml/foo.bar" will
  // output "/home/libkml/foo.bar".
  // Clients should use UriToFilename in preference to this to have the path
  // name style handled automatically.
  static bool UriToUnixFilename(const string& uri, string* output);

  // Converts a Windows URI to its corresponding Windows filename. Returns
  // false if output is NULL or on any internal error in converting the uri.
  // For example, calling this function on "file:///C:/home/libkml/foo.bar"
  // will output "C:\\home\\libkml\\foo.bar".
  // Clients should use UriToFilename in preference to this to have the path
  // name style handled automatically.
  static bool UriToWindowsFilename(const string& uri, string* output);

  // Converts a filename to its corresponding URI. The implementation is
  // platform-independent and handles either UNIX- or Windows-style path names
  // transparently. Returns false if output is NULL or on any internal
  // error in converting the uri.
  static bool FilenameToUri(const string& filename, string* output);

  // Converts a UNIX filename to its corresponding URI. Returns false if
  // output is NULL or on any internal error in converting the filename.
  // For example, calling this function on "/home/libkml/foo.bar" will output
  // "file:///home/libkml/foo.bar".
  // Clients should use FilenameToUri in preference to this to have the path
  // name style handled automatically.
  static bool UnixFilenameToUri(const string& filename,
                                string* output);

  // Converts a Windows filename to its corresponding URI. Returns false if
  // output is NULL or on any internal error in converting the filename.
  // For example, calling this function on "C:\\home\\libkml\\foo.bar" will
  // output "file:///C:/home/libkml/foo.bar".
  // Clients should use FilenameToUri in preference to this to have the path
  // name style handled automatically.
  static bool WindowsFilenameToUri(const string& filename,
                                   string* output);

  // This returns the scheme of the URI if one exists.
  bool GetScheme(string* scheme) const;

  // This returns the host of the URI if one exists.
  bool GetHost(string* host) const;

  // This returns the port of the URI if one exists.
  bool GetPort(string* port) const;

  // This returns the query of the URI if one exists.
  bool GetQuery(string* query) const;

  // This method returns the fragment portion of the URI into the given
  // string if such is supplied.  If no string is supplied or if there is no
  // fragment in this URI false is returned.  The fragment returned does not
  // include the '#' found in the corresponding URI.
  bool GetFragment(string* fragment) const;

  // This method returns true if the uri has a path.  If an output string
  // pointer is supplied the path is saved there.
  bool GetPath(string* path) const;

 private:
  // UriParserPrivate hides the internals of the underlying third party
  // uriparser types from clients of this header.
  boost::scoped_ptr<UriParserPrivate> uri_parser_private_;

  // No copy construction or assignment please.
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(UriParser);
};

}  // end namespace kmlbase

#endif  // KML_BASE_URI_PARSER_H__

