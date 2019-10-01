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

// This file contains the declarations for the abstract Geometry element
// and concrete coordinates, Point, LineString, LinearRing, Polygon,
// outerBoundaryIs, and innerBoundaryIs elements.

// In addition to classes for the abstract and concrete elements in the
// KML standard there are internal convenience classes used here to hold
// common code.  Each such class is named *GeometryCommon and follows
// this general pattern: constructor is protected, implements set,get,has,clear
// for the field it owns, and parses that field (implements AddElement).
// Each concrete element owns serialization of all fields for itself as per
// the order the KML standard.  The KML standard does not specify the common
// simple elements in an order that maps well to a type hierarchy hence
// the more typical pattern of abstract types serializing their own
// fields is not followed here.
//
// Here is a quick summary of the type hierarchy used and what fields
// are associated with each type:
//
// class Geometry : public Object
//   AbstractGeometryGroup in the KML standard.  No child elements.
// class AltitudeGeometryCommon : public Geometry
//   Geometry with <altitudeMode>
// class ExtrudeGeometryCommon : public AltitudeGeometryCommon
//   Geometry with <altitudeMode> + <extrude>
// class CoordinatesGeometryCommon : public ExtrudeGeometryCommon
//   Geometry with <altitudeMode> + <extrude> + <coordinates>
// class Point : public CoordinatesGeometryCommon
//   <Point> has <altitudeMode> + <extrude> + <coordinates>
// class LineCommon : public CoordinatesGeometryCommon
//   LineCommon has <altitudeMode> + <extrude> + <coordinates> + <tessellate>
// class LineString : public LineCommon
//   <LineString> is an instantiation of LineCommon
// class LinearRing : public LineCommon
//   <LinearRing> is an instantiation of LineCommon
// class BoundaryCommon : public Element
//   BoundaryCommon has <LinearRing>
// class OuterBoundaryIs : public BoundaryCommon
//  <outerBoundaryIs> is an instantiation of BoundaryCommon
// class InnerBoundaryIs : public BoundaryCommon
//  <innerBoundaryIs> is an instantiation of BoundaryCommon
// class Polygon : public ExtrudeGeometryCommon
//   <Polygon> has <altitudeMode> + <extrude> + <tessellate> +
//      <outerBoundaryIs> and N x <innerBoundaryIs>
// class MultiGeometry : public Geometry
// Note: class Model : public AltitudeGeometryCommon

#ifndef KML_DOM_GEOMETRY_H__
#define KML_DOM_GEOMETRY_H__

#include <vector>
#include "kml/base/util.h"
#include "kml/base/vec3.h"
#include "kml/dom/extendeddata.h"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/link.h"  // Remove when model.h is repaired.
#include "kml/dom/object.h"

namespace kmldom {

class Serializer;
class Visitor;
class VisitorDriver;

// <coordinates>
class Coordinates : public BasicElement<Type_coordinates> {
 public:
  virtual ~Coordinates();

  // The main KML-specific API
  void add_latlngalt(double latitude, double longitude, double altitude) {
    coordinates_array_.push_back(kmlbase::Vec3(longitude, latitude, altitude));
  }

  void add_latlng(double latitude, double longitude) {
    coordinates_array_.push_back(kmlbase::Vec3(longitude, latitude));
  }

  void add_vec3(const kmlbase::Vec3& vec3) {
    coordinates_array_.push_back(vec3);
  }

  size_t get_coordinates_array_size() const {
    return coordinates_array_.size();
  }

  const kmlbase::Vec3 get_coordinates_array_at(size_t index) const {
    return coordinates_array_[index];
  }

  // Internal methods used in parser.  Public for unittest purposes.
  // See .cc for more details.
  void Parse(const string& char_data);
  static bool ParseVec3(const char* coords, char** nextp, kmlbase::Vec3* vec);

  // This clears the internal coordinates array.
  void Clear() {
    coordinates_array_.clear();
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  Coordinates();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;

  std::vector<kmlbase::Vec3> coordinates_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Coordinates);
};

// OGC KML 2.2 Standard: 10.1 kml:AbstractGeometryGroup
// OGC KML 2.2 XSD: <element name="AbstractGeometryGroup"...
class Geometry : public Object {
 public:
  virtual ~Geometry();
  virtual KmlDomType Type() const { return Type_Geometry; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Geometry || Object::IsA(type);
  }

 protected:
  // Geometry is abstract.
  Geometry();

 private:
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Geometry);
};

// Internal convenience class for any Geometry with <altitudeMode>.
// This is not in the KML standard, hence there is no type info.
class AltitudeGeometryCommon : public Geometry {
 public:
  virtual ~AltitudeGeometryCommon();

 protected:
  AltitudeGeometryCommon();

 public:
  // <altitudeMode>
  int get_altitudemode() const { return altitudemode_; }
  bool has_altitudemode() const { return has_altitudemode_; }
  void set_altitudemode(int value) {
    altitudemode_ = value;
    has_altitudemode_ = true;
  }
  void clear_altitudemode() {
    altitudemode_ = ALTITUDEMODE_CLAMPTOGROUND;
    has_altitudemode_ = false;
  }

  // <gx:altitudeMode>
  int get_gx_altitudemode() const { return gx_altitudemode_; }
  bool has_gx_altitudemode() const { return has_gx_altitudemode_; }
  void set_gx_altitudemode(int value) {
    gx_altitudemode_ = value;
    has_gx_altitudemode_ = true;
  }
  void clear_gx_altitudemode() {
    gx_altitudemode_ = GX_ALTITUDEMODE_CLAMPTOSEAFLOOR;
    has_gx_altitudemode_ = false;
  }

  virtual void AddElement(const ElementPtr& element);

 private:
  int altitudemode_;
  bool has_altitudemode_;
  int gx_altitudemode_;
  bool has_gx_altitudemode_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AltitudeGeometryCommon);
};

// Internal convenience class for any Geometry with <altitudeMode> + <extrude>
// This is not in the KML standard, hence there is no type info.
class ExtrudeGeometryCommon : public AltitudeGeometryCommon {
 public:
  virtual ~ExtrudeGeometryCommon();

  // <extrude>
  bool get_extrude() const { return extrude_; }
  bool has_extrude() const { return has_extrude_; }
  void set_extrude(bool value) {
    extrude_ = value;
    has_extrude_ = true;
  }
  void clear_extrude() {
    extrude_ = false;
    has_extrude_ = false;
  }

 protected:
  ExtrudeGeometryCommon();
  virtual void AddElement(const ElementPtr& element);

 private:
  bool extrude_;
  bool has_extrude_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ExtrudeGeometryCommon);
};

// Internal convenience class for any Geometry with
// <altitudeMode> + <extrude> + <coordinates>.
// This is not in the KML standard, hence there is no type info.
class CoordinatesGeometryCommon : public ExtrudeGeometryCommon {
 public:
  virtual ~CoordinatesGeometryCommon();

 public:
  // <coordinates>
  const CoordinatesPtr& get_coordinates() const { return coordinates_; }
  bool has_coordinates() const { return coordinates_ != NULL; }
  void set_coordinates(const CoordinatesPtr& coordinates) {
    SetComplexChild(coordinates, &coordinates_);
  }
  void clear_coordinates() {
    set_coordinates(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void AcceptChildren(VisitorDriver* driver);

 protected:
  CoordinatesGeometryCommon();
  // Parser support
  virtual void AddElement(const ElementPtr& element);

 private:
  CoordinatesPtr coordinates_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(CoordinatesGeometryCommon);
};

// <Point>
class Point : public CoordinatesGeometryCommon {
 public:
  virtual ~Point();
  virtual KmlDomType Type() const { return Type_Point; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Point || Geometry::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  Point();
  friend class Serializer;
  void Serialize(Serializer& serializer) const;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Point);
};

// Internal convenience class for code common to LineString and LinearRing.
// This is not in the KML standard, hence there is no type info.
class LineCommon : public CoordinatesGeometryCommon {
 public:
  virtual ~LineCommon();

 public:
  // <tessellate>
  bool get_tessellate() const { return tessellate_; }
  bool has_tessellate() const { return has_tessellate_; }
  void set_tessellate(bool value) {
    tessellate_ = value;
    has_tessellate_ = true;
  }
  void clear_tessellate() {
    tessellate_ = false;
    has_tessellate_ = false;
  }

 protected:
  LineCommon();
  // Parser support
  virtual void AddElement(const ElementPtr& element);

 private:
  friend class Serializer;
  void Serialize(Serializer& serializer) const;
  bool tessellate_;
  bool has_tessellate_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(LineCommon);
};

// <LineString>
class LineString : public LineCommon {
 public:
  virtual ~LineString();
  virtual KmlDomType Type() const { return Type_LineString; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_LineString || Geometry::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  LineString();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(LineString);
};

// <LinearRing>
class LinearRing : public LineCommon {
 public:
  virtual ~LinearRing();
  virtual KmlDomType Type() const { return Type_LinearRing; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_LinearRing || Geometry::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  LinearRing();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(LinearRing);
};

// Internal class for code common to OuterBoundaryIs and InnerBoundaryIs.
// This is not in the KML standard, hence there is no type info.
class BoundaryCommon : public Element {
 public:
  virtual ~BoundaryCommon();

 public:
  const LinearRingPtr& get_linearring() const { return linearring_; }
  bool has_linearring() const { return linearring_ != NULL; }
  void set_linearring(const LinearRingPtr& linearring) {
    SetComplexChild(linearring, &linearring_);
  }
  void clear_linearring() {
    set_linearring(NULL);
  }

  // Parser support
  virtual void AddElement(const ElementPtr& element);

  // Visitor API methods, see visitor.h.
  virtual void AcceptChildren(VisitorDriver* driver);

 protected:
  BoundaryCommon();
  virtual void Serialize(Serializer& serializer) const;

 private:
  LinearRingPtr linearring_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(BoundaryCommon);
};

// <outerBoundaryIs>
class OuterBoundaryIs : public BoundaryCommon {
 public:
  virtual ~OuterBoundaryIs();
  virtual KmlDomType Type() const { return Type_outerBoundaryIs; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_outerBoundaryIs;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  OuterBoundaryIs();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(OuterBoundaryIs);
};

// <innerBoundaryIs>
class InnerBoundaryIs : public BoundaryCommon {
 public:
  virtual ~InnerBoundaryIs();
  virtual KmlDomType Type() const { return Type_innerBoundaryIs; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_innerBoundaryIs;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  InnerBoundaryIs();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(InnerBoundaryIs);
};

// <Polygon>
class Polygon : public ExtrudeGeometryCommon {
 public:
  virtual ~Polygon();
  virtual KmlDomType Type() const { return Type_Polygon; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Polygon || Geometry::IsA(type);
  }

  // <tessellate>
  bool get_tessellate() const { return tessellate_; }
  bool has_tessellate() const { return has_tessellate_; }
  void set_tessellate(bool value) {
    tessellate_ = value;
    has_tessellate_ = true;
  }
  void clear_tessellate() {
    tessellate_ = false;
    has_tessellate_ = false;
  }

  // <outerBoundaryIs>
  const OuterBoundaryIsPtr& get_outerboundaryis() const {
    return outerboundaryis_;
  }
  bool has_outerboundaryis() const { return outerboundaryis_ != NULL; }
  void set_outerboundaryis(const OuterBoundaryIsPtr& outerboundaryis) {
    SetComplexChild(outerboundaryis, &outerboundaryis_);
  }
  void clear_outerboundaryis() {
    set_outerboundaryis(NULL);
  }

  // <innerBoundaryIs>
  void add_innerboundaryis(const InnerBoundaryIsPtr& innerboundaryis) {
    AddComplexChild(innerboundaryis, &innerboundaryis_array_);
  }

  size_t get_innerboundaryis_array_size() const {
    return innerboundaryis_array_.size();
  }

  const InnerBoundaryIsPtr& get_innerboundaryis_array_at(size_t index) {
    return innerboundaryis_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Polygon();

  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);

  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;

  bool tessellate_;
  bool has_tessellate_;
  OuterBoundaryIsPtr outerboundaryis_;
  std::vector<InnerBoundaryIsPtr> innerboundaryis_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Polygon);
};

// <MultiGeometry>
class MultiGeometry : public Geometry {
 public:
  virtual ~MultiGeometry();
  virtual KmlDomType Type() const { return Type_MultiGeometry; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_MultiGeometry || Geometry::IsA(type);
  }

  // The main KML-specific API
  void add_geometry(const GeometryPtr& geometry);

  size_t get_geometry_array_size() const {
    return geometry_array_.size();
  }

  const GeometryPtr& get_geometry_array_at(size_t index) const {
    return geometry_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  MultiGeometry();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  std::vector<GeometryPtr> geometry_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(MultiGeometry);
};

// <gx:Track>
class GxTrack : public AltitudeGeometryCommon {
 public:
  virtual ~GxTrack();
  static KmlDomType ElementType() { return Type_GxTrack; }
  virtual KmlDomType Type() const { return ElementType(); }
  virtual bool IsA(KmlDomType type) const {
    return type == ElementType() || Geometry::IsA(type);
  }

  // <when>
  size_t get_when_array_size() {
    return when_array_.size();
  }
  void add_when(const string& when) {
    when_array_.push_back(when);
  }
  const string& get_when_array_at(size_t index) const {
    return when_array_[index];
  }

  // <gx:coord>
  size_t get_gx_coord_array_size() {
    return gx_coord_array_.size();
  }
  void add_gx_coord(const kmlbase::Vec3& gx_coord) {
    gx_coord_array_.push_back(gx_coord);
  }
  const kmlbase::Vec3& get_gx_coord_array_at(size_t index) const {
    return gx_coord_array_[index];
  }

  // <gx:angles>
  size_t get_gx_angles_array_size() {
    return gx_angles_array_.size();
  }
  void add_gx_angles(const kmlbase::Vec3& gx_angles) {
    gx_angles_array_.push_back(gx_angles);
  }
  const kmlbase::Vec3& get_gx_angles_array_at(size_t index) const {
    return gx_angles_array_[index];
  }

  // <Model>
  const ModelPtr& get_model() const { return model_; }
  void set_model(const ModelPtr& model) {
    SetComplexChild(model, &model_);
  }
  bool has_model() const { return model_ != NULL; }
  void clear_model() { set_model(NULL); }

  // <ExtendedData>
  const ExtendedDataPtr& get_extendeddata() const { return extendeddata_; }
  bool has_extendeddata() const { return extendeddata_ != NULL; }
  void set_extendeddata(const ExtendedDataPtr& extendeddata) {
    SetComplexChild(extendeddata, &extendeddata_);
  }
  void clear_extendeddata() {
    set_extendeddata(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

  // Internal methods used in parser.  Public for unittest purposes.
  // See .cc for more details.
  void Parse(const string& char_data, std::vector<kmlbase::Vec3>* out);

 private:
  friend class KmlFactory;
  GxTrack();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  std::vector<string> when_array_;
  std::vector<kmlbase::Vec3> gx_coord_array_;
  std::vector<kmlbase::Vec3> gx_angles_array_;
  ModelPtr model_;
  ExtendedDataPtr  extendeddata_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(GxTrack);
};

// <gx:MultiTrack>
class GxMultiTrack : public Geometry {
 public:
  virtual ~GxMultiTrack();
  static KmlDomType ElementType() { return Type_GxMultiTrack; }
  virtual KmlDomType Type() const { return ElementType(); }
  virtual bool IsA(KmlDomType type) const {
    return type == ElementType() || Geometry::IsA(type);
  }

  bool get_gx_interpolate() const { return gx_interpolate_; }
  bool has_gx_interpolate() const { return has_gx_interpolate_; }
  void set_gx_interpolate(bool value) {
    gx_interpolate_ = value;
    has_gx_interpolate_ = true;
  }
  void clear_gx_interpolate() {
    gx_interpolate_ = false;  // Default <gx:interpolate> is false.
    has_gx_interpolate_ = false;
  }

  void add_gx_track(const GxTrackPtr& gx_track);

  size_t get_gx_track_array_size() const {
    return gx_track_array_.size();
  }

  const GxTrackPtr& get_gx_track_array_at(size_t index) const {
    return gx_track_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  GxMultiTrack();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  bool gx_interpolate_;
  bool has_gx_interpolate_;
  std::vector<GxTrackPtr> gx_track_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(GxMultiTrack);
};


// HACK: the rest of this file contains what was in, and what should return to,
// kml/dom/model.h. GxTrack was added to this file, which has a <Model>. Since
// Model is defined in its own file, this double inclusion, coupled with the
// inline implementation of most methods in the headers, caused the builds of
// other dependent projects to break. The correct solution is to ensure that
// the headers are pure and all implementation is in the .cc files.

// <Location>
class Location : public Object {
 public:
  virtual ~Location();
  virtual KmlDomType Type() const { return Type_Location; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Location || Object::IsA(type);
  }

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

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  Location();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  double longitude_;
  bool has_longitude_;
  double latitude_;
  bool has_latitude_;
  double altitude_;
  bool has_altitude_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Location);
};

// <Orientation>
class Orientation : public Object {
 public:
  virtual ~Orientation();
  virtual KmlDomType Type() const { return Type_Orientation; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Orientation || Object::IsA(type);
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
  Orientation();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  double heading_;
  bool has_heading_;
  double tilt_;
  bool has_tilt_;
  double roll_;
  bool has_roll_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Orientation);
};

// <Scale>
class Scale : public Object {
 public:
  virtual ~Scale();
  virtual KmlDomType Type() const { return Type_Scale; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Scale || Object::IsA(type);
  }

  // <x>
  double get_x() const {
    return x_;
  }
  bool has_x() const {
    return has_x_;
  }
  void set_x(double x) {
    x_ = x;
    has_x_ = true;
  }
  void clear_x() {
    x_ = 1.0;
    has_x_ = false;
  }

  // <y>
  double get_y() const {
    return y_;
  }
  bool has_y() const {
    return has_y_;
  }
  void set_y(double y) {
    y_ = y;
    has_y_ = true;
  }
  void clear_y() {
    y_ = 1.0;
    has_y_ = false;
  }

  // <z>
  double get_z() const {
    return z_;
  }
  bool has_z() const {
    return has_z_;
  }
  void set_z(double z) {
    z_ = z;
    has_z_ = true;
  }
  void clear_z() {
    z_ = 1.0;
    has_z_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  Scale();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  double x_;
  bool has_x_;
  double y_;
  bool has_y_;
  double z_;
  bool has_z_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Scale);
};

// <Alias>
class Alias : public Object {
 public:
  virtual ~Alias();
  virtual KmlDomType Type() const { return Type_Alias; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Alias || Object::IsA(type);
  }

  // <targetHref>
  const string& get_targethref() const {
    return targethref_;
  }
  bool has_targethref() const {
    return has_targethref_;
  }
  void set_targethref(const string& targethref) {
    targethref_ = targethref;
    has_targethref_ = true;
  }
  void clear_targethref() {
    targethref_.clear();
    has_targethref_ = false;
  }

  // <sourceHref>
  const string& get_sourcehref() const {
    return sourcehref_;
  }
  bool has_sourcehref() const {
    return has_sourcehref_;
  }
  void set_sourcehref(const string& sourcehref) {
    sourcehref_ = sourcehref;
    has_sourcehref_ = true;
  }
  void clear_sourcehref() {
    sourcehref_.clear();
    has_sourcehref_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  Alias();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  string targethref_;
  bool has_targethref_;
  string sourcehref_;
  bool has_sourcehref_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Alias);
};

// <ResourceMap>
class ResourceMap : public Object {
 public:
  virtual ~ResourceMap();
  virtual KmlDomType Type() const { return Type_ResourceMap; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_ResourceMap || Object::IsA(type);
  }

  void add_alias(const AliasPtr& alias);

  size_t get_alias_array_size() const {
    return alias_array_.size();
  }

  const AliasPtr& get_alias_array_at(size_t index) const {
    return alias_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  ResourceMap();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  std::vector<AliasPtr> alias_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ResourceMap);
};

// <Model>
class Model : public AltitudeGeometryCommon {
 public:
  virtual ~Model();
  virtual KmlDomType Type() const { return Type_Model; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Model || Geometry::IsA(type);
  }

  // <Location>
  const LocationPtr& get_location() const { return location_; }
  bool has_location() const { return location_ != NULL; }
  void set_location(const LocationPtr& location) {
    SetComplexChild(location, &location_);
  }
  void clear_location() {
    set_location(NULL);
  }

  // <Orientation>
  const OrientationPtr& get_orientation() const { return orientation_; }
  bool has_orientation() const { return orientation_ != NULL; }
  void set_orientation(const OrientationPtr& orientation) {
    SetComplexChild(orientation, &orientation_);
  }
  void clear_orientation() {
    set_orientation(NULL);
  }

  // <Scale>
  const ScalePtr& get_scale() const { return scale_; }
  bool has_scale() const { return scale_ != NULL; }
  void set_scale(const ScalePtr& scale) {
    SetComplexChild(scale, &scale_);
  }
  void clear_scale() {
    set_scale(NULL);
  }

  // <Link>
  const LinkPtr& get_link() const { return link_; }
  bool has_link() const { return link_ != NULL; }
  void set_link(const LinkPtr& link) {
    SetComplexChild(link, &link_);
  }
  void clear_link() {
    set_link(NULL);
  }

  // <ResourceMap>
  const ResourceMapPtr& get_resourcemap() const { return resourcemap_; }
  bool has_resourcemap() const { return resourcemap_ != NULL; }
  void set_resourcemap(const ResourceMapPtr& resourcemap) {
    SetComplexChild(resourcemap, &resourcemap_);
  }
  void clear_resourcemap() {
    resourcemap_ = NULL;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Model();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  LocationPtr location_;
  OrientationPtr orientation_;
  ScalePtr scale_;
  LinkPtr link_;
  ResourceMapPtr resourcemap_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Model);
};

}  // namespace kmldom

#endif  // KML_DOM_GEOMETRY_H__

