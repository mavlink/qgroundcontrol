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

#ifndef KML_DOM_ABSTRACTVIEW_H__
#define KML_DOM_ABSTRACTVIEW_H__

#include "kml/dom/object.h"
#include "kml/dom/gx_timeprimitive.h"

namespace kmldom {

class Visitor;
class VisitorDriver;

// OGC KML 2.2 Standard: 14.1 kml:AbstractViewGroup
// OGC KML 2.2 XSD: <element name="AbstractViewGroup"...
class AbstractView : public Object {
 public:
  virtual ~AbstractView() {}
  virtual KmlDomType Type() const { return Type_AbstractView; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_AbstractView || Object::IsA(type);
  }

  // From kml:AbstractViewObjectExtensionGroup.
  const TimePrimitivePtr& get_gx_timeprimitive() const {
    return gx_timeprimitive_;
  }
  bool has_gx_timeprimitive() const { return gx_timeprimitive_ != NULL; }
  void set_gx_timeprimitive(const TimePrimitivePtr& gx_timeprimitive) {
    SetComplexChild(gx_timeprimitive, &gx_timeprimitive_);
  }
  void clear_gx_timeprimitive() {
    set_gx_timeprimitive(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void AcceptChildren(VisitorDriver* driver);

 protected:
  // AbstractView is abstract.
  AbstractView() {}
  virtual void AddElement(const ElementPtr& element);
  virtual void Serialize(Serializer& serializer) const;

 private:
  TimePrimitivePtr gx_timeprimitive_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AbstractView);
};

// This is an internal convenience class for code common to LookAt and Camera.
// This is not part of the OGC KML 2.2 standard.
class AbstractViewCommon : public AbstractView {
 public:
  // <longitude>
  double get_longitude() const {
    return longitude_;
  }
  bool has_longitude() const {
    return has_longitude_;
  }
  void set_longitude(double longitude) {
    longitude_ = longitude;
    has_longitude_ = true;
  }
  void clear_longitude() {
    longitude_ = 0.0;
    has_longitude_ = false;
  }

  // <latitude>
  double get_latitude() const {
    return latitude_;
  }
  bool has_latitude() const {
    return has_latitude_;
  }
  void set_latitude(double latitude) {
    latitude_ = latitude;
    has_latitude_ = true;
  }
  void clear_latitude() {
    latitude_ = 0.0;
    has_latitude_ = false;
  }

  // <altitude>
  double get_altitude() const {
    return altitude_;
  }
  bool has_altitude() const {
    return has_altitude_;
  }
  void set_altitude(double altitude) {
    altitude_ = altitude;
    has_altitude_ = true;
  }
  void clear_altitude() {
    altitude_ = 0.0;
    has_altitude_ = false;
  }

  // <heading>
  double get_heading() const {
    return heading_;
  }
  bool has_heading() const {
    return has_heading_;
  }
  void set_heading(double heading) {
    heading_ = heading;
    has_heading_ = true;
  }
  void clear_heading() {
    heading_ = 0.0;
    has_heading_ = false;
  }

  // <tilt>
  double get_tilt() const {
    return tilt_;
  }
  bool has_tilt() const {
    return has_tilt_;
  }
  void set_tilt(double tilt) {
    tilt_ = tilt;
    has_tilt_ = true;
  }
  void clear_tilt() {
    tilt_ = 0.0;
    has_tilt_ = false;
  }

  // <altitudeMode>
  int get_altitudemode() const {
    return altitudemode_;
  }
  bool has_altitudemode() const {
    return has_altitudemode_;
  }
  void set_altitudemode(int altitudemode) {
    altitudemode_ = altitudemode;
    has_altitudemode_ = true;
  }
  void clear_altitudemode() {
    altitudemode_ = ALTITUDEMODE_CLAMPTOGROUND;
    has_altitudemode_ = false;
  }

  // <gx:altitudeMode>
  // NOTE: In OGC KML 2.2 altitude mode is a group hence only one of
  // <altitudeMode> _OR_ <gx:altitudeMode> shall be used for XSD validation.
  int get_gx_altitudemode() const {
    return gx_altitudemode_;
  }
  bool has_gx_altitudemode() const {
    return has_gx_altitudemode_;
  }
  void set_gx_altitudemode(int gx_altitudemode) {
    gx_altitudemode_ = gx_altitudemode;
    has_gx_altitudemode_ = true;
  }
  void clear_gx_altitudemode() {
    gx_altitudemode_ = GX_ALTITUDEMODE_CLAMPTOSEAFLOOR;
    has_gx_altitudemode_ = false;
  }

 protected:
  // AbstractViewCommon is abstract.
  AbstractViewCommon();
  ~AbstractViewCommon() {}
  virtual void AddElement(const ElementPtr& element);
  virtual void SerializeBeforeR(Serializer& serializer) const;
  virtual void SerializeAfterR(Serializer& serializer) const;

 private:
  double longitude_;
  bool has_longitude_;
  double latitude_;
  bool has_latitude_;
  double altitude_;
  bool has_altitude_;
  double heading_;
  bool has_heading_;
  double tilt_;
  bool has_tilt_;
  int altitudemode_;
  bool has_altitudemode_;
  int gx_altitudemode_;
  bool has_gx_altitudemode_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AbstractViewCommon);
};

// <LookAt>
class LookAt : public AbstractViewCommon {
 public:
  virtual ~LookAt() {}
  virtual KmlDomType Type() const { return Type_LookAt; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_LookAt || AbstractView::IsA(type);
  }

  // <range>
  double get_range() const {
    return range_;
  }
  bool has_range() const {
    return has_range_;
  }
  void set_range(double range) {
    range_ = range;
    has_range_ = true;
  }
  void clear_range() {
    range_ = 0.0;
    has_range_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  LookAt();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  double range_;
  bool has_range_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(LookAt);
};

// <Camera>
class Camera : public AbstractViewCommon {
 public:
  virtual ~Camera() {}
  virtual KmlDomType Type() const { return Type_Camera; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Camera || AbstractView::IsA(type);
  }

  // <roll>
  double get_roll() const {
    return roll_;
  }
  bool has_roll() const {
    return has_roll_;
  }
  void set_roll(double roll) {
    roll_ = roll;
    has_roll_ = true;
  }
  void clear_roll() {
    roll_ = 0.0;
    has_roll_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  Camera();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  double roll_;
  bool has_roll_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Camera);
};

}  // end namespace kmldom

#endif  // KML_DOM_ABSTRACTVIEW_H__
