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

// This file contains the declaration of the abstract Vec2 element.

#ifndef KML_DOM_VEC2_H__
#define KML_DOM_VEC2_H__

#include "kml/dom/element.h"
#include "kml/dom/kml22.h"
#include "kml/base/util.h"

namespace kmlbase {
class Attributes;
}

namespace kmldom {

class Serializer;
class Visitor;

// OGC KML 2.2 Standard: 16.21 kml:vec2Type
// OGC KML 2.2 XSD: <complexType name="vec2Type"...
class Vec2 : public Element {
 public:
  virtual ~Vec2();
  virtual KmlDomType Type() const { return Type_Vec2; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Vec2;
  }

  double get_x() const { return x_; }
  bool has_x() const { return has_x_; }
  void set_x(double value) {
    x_ = value;
    has_x_ = true;
  }
  void clear_x() {
    x_ = 1.0;
    has_x_ = false;
  }

  double get_y() const { return y_; }
  bool has_y() const { return has_y_; }
  void set_y(double value) {
    y_ = value;
    has_y_ = true;
  }
  void clear_y() {
    y_ = 1.0;
    has_y_ = false;
  }

  int get_xunits() const { return xunits_; }
  bool has_xunits() const { return has_xunits_; }
  void set_xunits(int value) {
    xunits_ = value;
    has_xunits_ = true;
  }
  void clear_xunits() {
    xunits_ = false;
    has_xunits_ = false;
  }

  int get_yunits() const { return yunits_; }
  bool has_yunits() const { return has_yunits_; }
  void set_yunits(int value) {
    yunits_ = value;
    has_yunits_ = true;
  }
  void clear_yunits() {
    yunits_ = false;
    has_yunits_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 protected:
  // Vec2 is abstract, derived class access only.
  Vec2();
  virtual void ParseAttributes(kmlbase::Attributes* attributes);
  virtual void SerializeAttributes(kmlbase::Attributes* attributes) const;
  void Serialize(Serializer& serializer) const;

 private:
  double x_;
  bool has_x_;
  double y_;
  bool has_y_;
  int xunits_;
  bool has_xunits_;
  int yunits_;
  bool has_yunits_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Vec2);
};

}  // end namespace kmldom

#endif  // KML_DOM_VEC2_H__
