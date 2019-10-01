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

// This file contains the declarations for the abstract Overlay element
// and the concrete GroundOverlay, ScreenOverlay, and PhotoOverlay elements
// and their child elements LatLonBox, OverlayXY, ScreenXY, RotationXY,
// Size, ViewVolume, and ImagePyramid.

#ifndef KML_DOM_OVERLAY_H__
#define KML_DOM_OVERLAY_H__

#include "kml/base/color32.h"
#include "kml/dom/abstractlatlonbox.h"
#include "kml/dom/feature.h"
#include "kml/dom/geometry.h"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/link.h"
#include "kml/dom/object.h"
#include "kml/dom/vec2.h"

namespace kmldom {

class Serializer;
class Visitor;
class VisitorDriver;

// OGC KML 2.2 Standard: 11.1 kml:AbstractOverlayGroup
// OGC KML 2.2 XSD: <element name="AbstractOverlayGroup"...
class Overlay : public Feature {
 public:
  virtual ~Overlay();
  virtual KmlDomType Type() const { return Type_Overlay; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Overlay || Feature::IsA(type);
  }

  // <color>
  const kmlbase::Color32& get_color() const {
    return color_;
  }
  bool has_color() const {
    return has_color_;
  }
  void set_color(const kmlbase::Color32& color) {
    color_ = color;
    has_color_ = true;
  }
  void clear_color() {
    color_ = kmlbase::Color32(0xffffffff);
    has_color_ = false;
  }

  // <drawOrder>
  int get_draworder() const {
    return draworder_;
  }
  bool has_draworder() const {
    return has_draworder_;
  }
  void set_draworder(int draworder) {
    draworder_ = draworder;
    has_draworder_ = true;
  }
  void clear_draworder() {
    draworder_ = 0;
    has_draworder_ = false;
  }

  // <Icon>
  const IconPtr& get_icon() const { return icon_; }
  bool has_icon() const { return icon_ != NULL; }
  void set_icon(const IconPtr& icon) {
    SetComplexChild(icon, &icon_);
  }
  void clear_icon() {
    set_icon(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void AcceptChildren(VisitorDriver* driver);

 protected:
  // Overlay is abstract.
  Overlay();
  virtual void AddElement(const ElementPtr& element);
  virtual void Serialize(Serializer& serializer) const;

 private:
  kmlbase::Color32 color_;
  bool has_color_;
  int draworder_;
  bool has_draworder_;
  IconPtr icon_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Overlay);
};

// <LatLonBox>
class LatLonBox : public AbstractLatLonBox {
 public:
  virtual ~LatLonBox();
  virtual KmlDomType Type() const { return Type_LatLonBox; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_LatLonBox || AbstractLatLonBox::IsA(type);
  }

  // <rotation>
  double get_rotation() const {
    return rotation_;
  }
  bool has_rotation() const {
    return has_rotation_;
  }
  void set_rotation(double rotation) {
    rotation_ = rotation;
    has_rotation_ = true;
  }
  void clear_rotation() {
    rotation_ = 0.0;
    has_rotation_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  LatLonBox();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;

 private:
  double rotation_;
  bool has_rotation_;

  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(LatLonBox);
};

// <gx:LatLonQuad>
class GxLatLonQuad : public Object {
 public:
  virtual ~GxLatLonQuad();
  static KmlDomType ElementType() { return Type_GxLatLonQuad; }
  virtual KmlDomType Type() const { return ElementType(); }
  virtual bool IsA(KmlDomType type) const {
    return type == ElementType() || Object::IsA(type);
  }

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
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  GxLatLonQuad();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  CoordinatesPtr coordinates_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(GxLatLonQuad);
};

// <GroundOverlay>
class GroundOverlay : public Overlay {
 public:
  virtual ~GroundOverlay();
  virtual KmlDomType Type() const { return Type_GroundOverlay; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_GroundOverlay || Overlay::IsA(type);
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

  // <LatLonBox>
  const LatLonBoxPtr& get_latlonbox() const { return latlonbox_; }
  bool has_latlonbox() const { return latlonbox_ != NULL; }
  void set_latlonbox(const LatLonBoxPtr& latlonbox) {
    SetComplexChild(latlonbox, &latlonbox_);
  }
  void clear_latlonbox() {
    set_latlonbox(NULL);
  }

  // <gx:LatLonQuad>
  const GxLatLonQuadPtr& get_gx_latlonquad() const { return gx_latlonquad_; }
  bool has_gx_latlonquad() const { return gx_latlonquad_ != NULL; }
  void set_gx_latlonquad(const GxLatLonQuadPtr& gx_latlonquad) {
    SetComplexChild(gx_latlonquad, &gx_latlonquad_);
  }
  void clear_gx_latlonquad() {
    set_gx_latlonquad(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  GroundOverlay();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;

 private:
  double altitude_;
  bool has_altitude_;
  int altitudemode_;
  bool has_altitudemode_;
  int gx_altitudemode_;
  bool has_gx_altitudemode_;
  LatLonBoxPtr latlonbox_;
  GxLatLonQuadPtr gx_latlonquad_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(GroundOverlay);
};

// <overlayXY>
class OverlayXY : public Vec2 {
 public:
  virtual ~OverlayXY();
  virtual KmlDomType Type() const { return Type_overlayXY; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_overlayXY || Vec2::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  OverlayXY();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(OverlayXY);
};

// <screenXY>
class ScreenXY : public Vec2 {
 public:
  virtual ~ScreenXY();
  virtual KmlDomType Type() const { return Type_screenXY; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_screenXY || Vec2::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  ScreenXY();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ScreenXY);
};

// <rotationXY>
class RotationXY : public Vec2 {
 public:
  virtual ~RotationXY();
  virtual KmlDomType Type() const { return Type_rotationXY; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_rotationXY || Vec2::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  RotationXY();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(RotationXY);
};

// <size>
class Size : public Vec2 {
 public:
  virtual ~Size();
  virtual KmlDomType Type() const { return Type_size; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_size || Vec2::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  Size();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Size);
};

// <ScreenOverlay>
class ScreenOverlay : public Overlay {
 public:
  virtual ~ScreenOverlay();
  virtual KmlDomType Type() const { return Type_ScreenOverlay; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_ScreenOverlay || Overlay::IsA(type);
  }

  // <overlayXY>
  const OverlayXYPtr& get_overlayxy() const { return overlayxy_; }
  bool has_overlayxy() const { return overlayxy_ != NULL; }
  void set_overlayxy(const OverlayXYPtr& overlayxy) {
    SetComplexChild(overlayxy, &overlayxy_);
  }
  void clear_overlayxy() {
    set_overlayxy(NULL);
  }

  // <screenXY>
  const ScreenXYPtr& get_screenxy() const { return screenxy_; }
  bool has_screenxy() const { return screenxy_ != NULL; }
  void set_screenxy(const ScreenXYPtr& screenxy) {
    SetComplexChild(screenxy, &screenxy_);
  }
  void clear_screenxy() {
    set_screenxy(NULL);
  }

  // <rotationXY>
  const RotationXYPtr& get_rotationxy() const { return rotationxy_; }
  bool has_rotationxy() const { return rotationxy_ != NULL; }
  void set_rotationxy(const RotationXYPtr& rotationxy) {
    SetComplexChild(rotationxy, &rotationxy_);
  }
  void clear_rotationxy() {
    set_rotationxy(NULL);
  }

  // <size>
  const SizePtr& get_size() const { return size_; }
  bool has_size() const { return size_ != NULL; }
  void set_size(const SizePtr& size) {
    SetComplexChild(size, &size_);
  }
  void clear_size() {
    set_size(NULL);
  }

  // <rotation>
  double get_rotation() const {
    return rotation_;
  }
  bool has_rotation() const {
    return has_rotation_;
  }
  void set_rotation(double rotation) {
    rotation_ = rotation;
    has_rotation_ = true;
  }
  void clear_rotation() {
    rotation_ = 0.0;
    has_rotation_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  ScreenOverlay();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  OverlayXYPtr overlayxy_;
  ScreenXYPtr screenxy_;
  RotationXYPtr rotationxy_;
  SizePtr size_;
  double rotation_;
  bool has_rotation_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ScreenOverlay);
};

// <ViewVolume>
class ViewVolume : public Object {
 public:
  virtual ~ViewVolume();
  virtual KmlDomType Type() const { return Type_ViewVolume; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_ViewVolume || Object::IsA(type);
  }

  // <leftFov>
  double get_leftfov() const {
    return leftfov_;
  }
  bool has_leftfov() const {
    return has_leftfov_;
  }
  void set_leftfov(double leftfov) {
    leftfov_ = leftfov;
    has_leftfov_ = true;
  }
  void clear_leftfov() {
    leftfov_ = 0.0;
    has_leftfov_ = false;
  }

  // <rightFov>
  double get_rightfov() const {
    return rightfov_;
  }
  bool has_rightfov() const {
    return has_rightfov_;
  }
  void set_rightfov(double rightfov) {
    rightfov_ = rightfov;
    has_rightfov_ = true;
  }
  void clear_rightfov() {
    rightfov_ = 0.0;
    has_rightfov_ = false;
  }

  // <bottomFov>
  double get_bottomfov() const {
    return bottomfov_;
  }
  bool has_bottomfov() const {
    return has_bottomfov_;
  }
  void set_bottomfov(double altitude) {
    bottomfov_ = altitude;
    has_bottomfov_ = true;
  }
  void clear_bottomfov() {
    bottomfov_ = 0.0;
    has_bottomfov_ = false;
  }

  // <topFov>
  double get_topfov() const {
    return topfov_;
  }
  bool has_topfov() const {
    return has_topfov_;
  }
  void set_topfov(double topfov) {
    topfov_ = topfov;
    has_topfov_ = true;
  }
  void clear_topfov() {
    topfov_ = 0.0;
    has_topfov_ = false;
  }

  // <near>
  double get_near() const {
    return near_;
  }
  bool has_near() const {
    return has_near_;
  }
  void set_near(double val) {
    near_ = val;
    has_near_ = true;
  }
  void clear_near() {
    near_ = 0.0;
    has_near_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  ViewVolume();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  double leftfov_;
  bool has_leftfov_;
  double rightfov_;
  bool has_rightfov_;
  double bottomfov_;
  bool has_bottomfov_;
  double topfov_;
  bool has_topfov_;
  double near_;
  bool has_near_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ViewVolume);
};

// <ImagePyramid>
class ImagePyramid : public Object {
 public:
  virtual ~ImagePyramid();
  virtual KmlDomType Type() const { return Type_ImagePyramid; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_ImagePyramid || Object::IsA(type);
  }

  // <tileSize>
  int get_tilesize() const {
    return tilesize_;
  }
  bool has_tilesize() const {
    return has_tilesize_;
  }
  void set_tilesize(int tilesize) {
    tilesize_ = tilesize;
    has_tilesize_ = true;
  }
  void clear_tilesize() {
    tilesize_ = 256;
    has_tilesize_ = false;
  }

  // <maxWidth>
  int get_maxwidth() const {
    return maxwidth_;
  }
  bool has_maxwidth() const {
    return has_maxwidth_;
  }
  void set_maxwidth(int maxwidth) {
    maxwidth_ = maxwidth;
    has_maxwidth_ = true;
  }
  void clear_maxwidth() {
    maxwidth_ = 0;
    has_maxwidth_ = false;
  }

  // <maxHeight>
  int get_maxheight() const {
    return maxheight_;
  }
  bool has_maxheight() const {
    return has_maxheight_;
  }
  void set_maxheight(int altitude) {
    maxheight_ = altitude;
    has_maxheight_ = true;
  }
  void clear_maxheight() {
    maxheight_ = 0;
    has_maxheight_ = false;
  }

  // <gridOrigin>
  int get_gridorigin() const {
    return gridorigin_;
  }
  bool has_gridorigin() const {
    return has_gridorigin_;
  }
  void set_gridorigin(int gridorigin) {
    gridorigin_ = gridorigin;
    has_gridorigin_ = true;
  }
  void clear_gridorigin() {
    gridorigin_ = GRIDORIGIN_LOWERLEFT;
    has_gridorigin_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  ImagePyramid();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  int tilesize_;
  bool has_tilesize_;
  int maxwidth_;
  bool has_maxwidth_;
  int maxheight_;
  bool has_maxheight_;
  int gridorigin_;
  bool has_gridorigin_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ImagePyramid);
};

// <PhotoOverlay>
class PhotoOverlay : public Overlay {
 public:
  virtual ~PhotoOverlay();
  virtual KmlDomType Type() const { return Type_PhotoOverlay; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_PhotoOverlay || Overlay::IsA(type);
  }

  // <rotation>
  double get_rotation() const {
    return rotation_;
  }
  bool has_rotation() const {
    return has_rotation_;
  }
  void set_rotation(double rotation) {
    rotation_ = rotation;
    has_rotation_ = true;
  }
  void clear_rotation() {
    rotation_ = 0.0;
    has_rotation_ = false;
  }

  // <ViewVolume>
  const ViewVolumePtr& get_viewvolume() const { return viewvolume_; }
  bool has_viewvolume() const { return viewvolume_ != NULL; }
  void set_viewvolume(const ViewVolumePtr& viewvolume) {
    SetComplexChild(viewvolume, &viewvolume_);
  }
  void clear_viewvolume() {
    set_viewvolume(NULL);
  }

  // <ImagePyramid>
  const ImagePyramidPtr& get_imagepyramid() const { return imagepyramid_; }
  bool has_imagepyramid() const { return imagepyramid_ != NULL; }
  void set_imagepyramid(const ImagePyramidPtr& imagepyramid) {
    SetComplexChild(imagepyramid, &imagepyramid_);
  }
  void clear_imagepyramid() {
    set_imagepyramid(NULL);
  }

  // <Point>
  const PointPtr& get_point() const { return point_; }
  bool has_point() const { return point_ != NULL; }
  void set_point(const PointPtr& point) {
    SetComplexChild(point, &point_);
  }
  void clear_point() {
    set_point(NULL);
  }

  // <shape>
  int get_shape() const {
    return shape_;
  }
  bool has_shape() const {
    return has_shape_;
  }
  void set_shape(int shape) {
    shape_ = shape;
    has_shape_ = true;
  }
  void clear_shape() {
    shape_ = SHAPE_RECTANGLE;
    has_shape_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  PhotoOverlay();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  double rotation_;
  bool has_rotation_;
  ViewVolumePtr viewvolume_;
  ImagePyramidPtr imagepyramid_;
  PointPtr point_;
  int shape_;
  bool has_shape_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(PhotoOverlay);
};

}  // end namespace kmldom

#endif  // KML_DOM_OVERLAY_H__
