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

// This file contains the declaration of the internal KmlUri class.  Do not use
// this class in application code.  Use the functions declared in kml_uri.h.


#ifndef KML_ENGINE_KML_URI_INTERNAL_H__
#define KML_ENGINE_KML_URI_INTERNAL_H__

#include "boost/scoped_ptr.hpp"
#include "kml/base/util.h"

// Forward declare to avoid including uri_parser.h in app code.
namespace kmlbase {
class UriParser;
}

namespace kmlengine {

// The main purpose of the KmlUri class is to hold the URI state for a given
// fetch.  This state is principally a base url and a relative target to fetch.
// Ideally any URI stands alone, however, the two-level fetch system used for
// relative KMZ references requires the target reference to be retained
// to be resolved against either the full URI of the base (typically that of
// the KmlFile in the KML Engine) or the URI of the KMZ archive containing
// the KML file, in that order.  For more details and examples see kml_uri.cc
// NOTE: This is an internal class.  Do not use in application code.
// Applications should use KmlFile (where the API provides the means to pass
// a base URI and target URI for relative fetches).
class KmlUri {
 public:
  // The base is a full absolute URI including scheme.  The base is typically
  // the URI of a KML file as maintained in KmlFile::get_url().  For example,
  // http://host.com/dir/path.kml, or http://host.com/dir/path.kmz/doc.kml.
  // (Note that a "bare" KMZ reference here does _not_ automatically imply
  // "the KML file" within the KMZ.  See the note above about this being and
  // internal class).  The target is a relative or abstolue URI typically the
  // raw content of any <href>, <styleUrl>, schemaUrl=, <targetHref>,
  // <a href="...">, or <img href="..."> within the KmlFile.  However, there
  // is no specific knowlege of any KML or HTML element within this class.
  static KmlUri* CreateRelative(const string& base,
                                const string& target);

  ~KmlUri();

  bool is_kmz() const {
    return is_kmz_;
  }

  const string& get_target() const {
    return target_;
  }

  const string& get_url() const {
    return url_;
  }

  const string& get_kmz_url() const {
    return kmz_url_;
  }

  const string& get_path_in_kmz() const {
    return path_in_kmz_;
  }

  // TODO Ideally this class has no non-const methods.  No module should alter
  // a KmlUri.  Instead a new one should be created as needed.
  void set_path_in_kmz(const string path_in_kmz) {
    path_in_kmz_ = path_in_kmz;
    url_ = kmz_url_ + "/" + path_in_kmz;
  }

 private:
  // Private constructor.  Use static Create() method.
  // TODO streamline this with the Create method.
  KmlUri(const string& base, const string& target);

  bool is_kmz_;  // TODO should this be is_relative_kmz_?
  const string base_;
  const string target_;
  // TODO use UriParser's throughout _or_ string, not both.
  boost::scoped_ptr<kmlbase::UriParser> target_uri_;

  string url_;

  // TODO this is too complex.  Better might be to create a new KmlUri for
  // a new fetch.
  string kmz_url_;
  string path_in_kmz_;

  // No copy construction or assignment please.
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(KmlUri);
};

}  // end namespace kmlengine

#endif  // KML_ENGINE_KML_URI_INTERNAL_H__
