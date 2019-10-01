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

// This file declares the NetworkLink element.

#ifndef KML_DOM_NETWORKLINK_H__
#define KML_DOM_NETWORKLINK_H__

#include "kml/dom/feature.h"
#include "kml/dom/link.h"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/base/util.h"

namespace kmldom {

class Visitor;
class VisitorDriver;

// <NetworkLink>
class NetworkLink : public Feature {
 public:
  virtual ~NetworkLink();
  virtual KmlDomType Type() const { return Type_NetworkLink; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_NetworkLink || Feature::IsA(type);
  }

  // <refreshVisibility>
  bool get_refreshvisibility() const { return refreshvisibility_; }
  bool has_refreshvisibility() const { return has_refreshvisibility_; }
  void set_refreshvisibility(bool value) {
    refreshvisibility_ = value;
    has_refreshvisibility_ = true;
  }
  void clear_refreshvisibility() {
    refreshvisibility_ = false;
    has_refreshvisibility_ = false;
  }

  // <flyToView>
  bool get_flytoview() const { return flytoview_; }
  bool has_flytoview() const { return has_flytoview_; }
  void set_flytoview(bool value) {
    flytoview_ = value;
    has_flytoview_ = true;
  }
  void clear_flytoview() {
    flytoview_ = false;
    has_flytoview_ = false;
  }

  // <Link>
  // <Url> is deprecated, no API access
  const LinkPtr& get_link() const { return link_; }
  bool has_link() const { return link_ != NULL; }
  void set_link(const LinkPtr& link) {
    SetComplexChild(link, &link_);
  }
  void clear_link() {
    set_link(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  NetworkLink();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  bool refreshvisibility_;
  bool has_refreshvisibility_;
  bool flytoview_;
  bool has_flytoview_;
  LinkPtr link_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(NetworkLink);
};

}  // namespace kmldom

#endif  // KML_DOM_NETWORKLINK_H__
