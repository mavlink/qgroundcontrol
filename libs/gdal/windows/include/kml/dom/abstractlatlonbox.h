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

#ifndef KML_DOM_ABSTRACTLATLONBOX_H__
#define KML_DOM_ABSTRACTLATLONBOX_H__

#include "kml/dom/kml22.h"
#include "kml/dom/object.h"

namespace kmldom {

class Element;
class Serializer;

// OGC KML 2.2 Standard: 9.14 kml:AbstractLatLonAltBox
// OGC KML 2.2 XSD: <complexType name="AbstractLatLonBoxType" abstract="true">
class AbstractLatLonBox : public Object {
 public:
  virtual ~AbstractLatLonBox();
  virtual KmlDomType Type() const { return Type_AbstractLatLonBox; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_AbstractLatLonBox || Object::IsA(type);
  }

  // <north>
  double get_north() const {
    return north_;
  }
  bool has_north() const {
    return has_north_;
  }
  void set_north(double north) {
    north_ = north;
    has_north_ = true;
  }
  void clear_north() {
    north_ = 180.0;
    has_north_ = false;
  }

  // <south>
  double get_south() const {
    return south_;
  }
  bool has_south() const {
    return has_south_;
  }
  void set_south(double south) {
    south_ = south;
    has_south_ = true;
  }
  void clear_south() {
    south_ = -180.0;
    has_south_ = false;
  }

  // <east>
  double get_east() const {
    return east_;
  }
  bool has_east() const {
    return has_east_;
  }
  void set_east(double south) {
    east_ = south;
    has_east_ = true;
  }
  void clear_east() {
    east_ = 180.0;
    has_east_ = false;
  }

  // <west>
  double get_west() const {
    return west_;
  }
  bool has_west() const {
    return has_west_;
  }
  void set_west(double south) {
    west_ = south;
    has_west_ = true;
  }
  void clear_west() {
    west_ = -180.0;
    has_west_ = false;
  }

 protected:
  // Abstract element.  Access for derived types only.
  AbstractLatLonBox();
  virtual void AddElement(const ElementPtr& element);
  virtual void Serialize(Serializer& serializer) const;

 private:
  double north_;
  bool has_north_;
  double south_;
  bool has_south_;
  double east_;
  bool has_east_;
  double west_;
  bool has_west_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AbstractLatLonBox);
};

}  // end namespace kmldom

#endif  // KML_DOM_ABSTRACTLATLONBOX_H__
