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

// This is the declaration of the abstract element Object.

#ifndef KML_DOM_OBJECT_H__
#define KML_DOM_OBJECT_H__

#include "kml/dom/element.h"
#include "kml/dom/kml22.h"
#include "kml/base/util.h"

namespace kmlbase {
class Attributes;
}

namespace kmldom {

// OGC KML 2.2 Standard: 8.1 kml:AbstractObjectGroup
// OGC KML 2.2 XSD: <element name="AbstractObjectGroup"...
class Object : public Element {
 public:
  virtual ~Object();
  KmlDomType Type() const { return Type_Object; }
  bool IsA(KmlDomType type) const {
    return type == Type_Object;
  }

  const string& get_id() const { return id_; }
  bool has_id() const { return has_id_; }
  void set_id(const string& value) {
    id_ = value;
    has_id_ = true;
  }
  void clear_id() {
    id_.clear();
    has_id_ = false;
  }

  const string& get_targetid() const { return targetid_; }
  bool has_targetid() const { return has_targetid_; }
  void set_targetid(const string& targetid) {
    targetid_ = targetid;
    has_targetid_ = true;
  }
  void clear_targetid() {
    targetid_.clear();
    has_targetid_ = false;
  }

 protected:
  // Object is abstract, derived class access only.
  Object();
  virtual void AddElement(const ElementPtr& element);
  virtual void ParseAttributes(kmlbase::Attributes* attributes);
  virtual void SerializeAttributes(kmlbase::Attributes* attributes) const;

 private:
  string id_;
  bool has_id_;
  string targetid_;
  bool has_targetid_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Object);
};

}  // namespace kmldom

#endif  // KML_DOM_OBJECT_H__
