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

// This file contains the definition of the Href class used to parse a
// KML-style URL such as that found in <href> and <styleUrl>.
// TODO: methods to parse/assemble a full RFC 1808 URL:
// TODO: <scheme>://<net_loc>/<path>;<params>?<query>#<fragment>
// TODO: "...;params..." is not used in KML

#ifndef KML_ENGINE_HREF_H__
#define KML_ENGINE_HREF_H__

#include "kml/base/util.h"

namespace kmlengine {

// This class parses a KML URL.  Basic usage at present:
//   Href href("#object-id");
//   if (href.HasFragment()) {
//     return href.Fragment();  // "object-id"
//   }

class Href {
 public:
  Href() {}
  // Construct from the contents of <href>
  Href(const string& href) {
    Parse(href);
  }

  bool IsRelative() const {
    return !has_scheme() && !has_net_loc();
  }

  bool IsRelativePath() const {
    return !has_scheme() && !has_net_loc() && has_path();
  }

  bool IsFragmentOnly() const {
    return has_fragment() && !has_scheme() && !has_net_loc() && !has_path();
  }

  const string& get_scheme() const {
    return scheme_;
  }
  bool has_scheme() const {
    return !scheme_.empty();
  }
  void set_scheme(const string& scheme) {
    scheme_ = scheme;
  }
  void clear_scheme() {
    scheme_.clear();
  }

  const string& get_net_loc() const {
    return net_loc_;
  }
  bool has_net_loc() const {
    return !net_loc_.empty();
  }
  void set_net_loc(const string& net_loc) {
    net_loc_ = net_loc;
  }
  void clear_net_loc() {
    net_loc_.clear();
  }

  const string& get_path() const {
    return path_;
  }
  bool has_path() const {
    return !path_.empty();
  }
  void set_path(const string& path) {
    path_ = path;
  }
  void clear_path() {
    path_.clear();
  }

  const string& get_fragment() const {
    return fragment_;
  }
  bool has_fragment() const {
    return !fragment_.empty();
  }
  void set_fragment(const string& fragment) {
    fragment_ = fragment;
  }
  void clear_fragment() {
    fragment_.clear();
  }

 private:
  void Parse(const string& href);
  size_t ParseScheme(const string& href);
  size_t ParseNetLoc(const string& href);

  // These names match RFC 1808:
  // <scheme>://<net_loc>/<path>;<params>?<query>#<fragment>
  // Note: params is unused in a KML URL.
  string scheme_;
  string net_loc_;
  string path_;
  string query_;
  string fragment_;
};

}  // end namespace kmlengine

#endif  // KML_ENGINE_HREF_H__
