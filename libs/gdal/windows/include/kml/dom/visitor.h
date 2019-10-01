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

// WARNING: THE VISITOR API IMPLEMENTED IN THIS CLASS IS EXPERIMENTAL AND
// SUBJECT TO CHANGE WITHOUT WARNING.

#ifndef KML_DOM_VISITOR_H__
#define KML_DOM_VISITOR_H__

#include <vector>
#include "kml/base/util.h"
#include "kml/dom/kml_ptr.h"

namespace kmldom {

// A base class implementing a visitor for elements in a kml dom hierarchy.
// This class should be extended to implement specific visitors by overriding
// the approriate VisitXxx() methods.
//
// The Visitor base class will ensure that the expected visit methods are
// invoked even if the element being visited is a sub-type of the element for
// which VisitXxx() was overridden. For example, if VisitContainer() is
// overridden by a subclass then it will be called for any Container elements
// visited, such as Document or Folder.
//
// If you wish to visit an element for several types (for example Container and
// Feature) using a single visitor, the sub-type visit methods should invoke
// their parent class method:
//
// void MyVisitor::VisitContainer(const ContainerPtr& container) {
//   // do stuff
//   Visitor::VisitContainer(container);  // calls VisitFeature()
// }
//
// A visitation over an element hierarchy is controlled by a VisitorDriver
// instance. The choice of driver can affect the order in which elements are
// visited and it is up to the user to select an appropriate driver for their
// needs. A visitor has no requirement to manage the visitation of its child
// elements as this is handled by the chosen driver, although a visitor is free
// to operate on its child elements directly if it so chooses.
//
// In typical usage processing an element hierarchy might look something like:
//
// const KmlPtr& root = GetRootElement();
// MyVisitor visitor();
// SimplePreorderDriver(visitor).Visit(root);
// ProcessResults(visitor.GetResults());
//
class Visitor {
 protected:
  Visitor();

 public:
  virtual ~Visitor();

  virtual void VisitElement(const ElementPtr& node);

  virtual void VisitAbstractLatLonBox(
      const AbstractLatLonBoxPtr& element);

  virtual void VisitAbstractLink(
      const AbstractLinkPtr& element);

  virtual void VisitAbstractView(
      const AbstractViewPtr& element);

  virtual void VisitAlias(
      const AliasPtr& element);

  virtual void VisitBalloonStyle(
      const BalloonStylePtr& element);

  virtual void VisitBasicLink(
      const BasicLinkPtr& element);

  virtual void VisitCamera(
      const CameraPtr& element);

  virtual void VisitChange(
      const ChangePtr& element);

  virtual void VisitColorStyle(
      const ColorStylePtr& element);

  virtual void VisitContainer(
      const ContainerPtr& element);

  virtual void VisitCoordinates(
      const CoordinatesPtr& element);

  virtual void VisitCreate(
      const CreatePtr& element);

  virtual void VisitData(
      const DataPtr& element);

  virtual void VisitDelete(
      const DeletePtr& element);

  virtual void VisitDocument(
      const DocumentPtr& element);

  virtual void VisitExtendedData(
      const ExtendedDataPtr& element);

  virtual void VisitFeature(
      const FeaturePtr& element);

  virtual void VisitField(
      const FieldPtr& element);

  virtual void VisitFolder(
      const FolderPtr& element);

  virtual void VisitGeometry(
      const GeometryPtr& element);

  virtual void VisitGroundOverlay(
      const GroundOverlayPtr& element);

  virtual void VisitGxAnimatedUpdate(
      const GxAnimatedUpdatePtr& element);

  virtual void VisitGxFlyTo(
      const GxFlyToPtr& element);

  virtual void VisitGxLatLonQuad(
      const GxLatLonQuadPtr& element);

  virtual void VisitGxMultiTrack(
      const GxMultiTrackPtr& element);

  virtual void VisitGxPlaylist(
      const GxPlaylistPtr& element);

  virtual void VisitGxSimpleArrayData(
      const GxSimpleArrayDataPtr& element);

  virtual void VisitGxSimpleArrayField(
      const GxSimpleArrayFieldPtr& element);

  virtual void VisitGxSoundCue(
      const GxSoundCuePtr& element);

  virtual void VisitGxTimeSpan(
      const GxTimeSpanPtr& element);

  virtual void VisitGxTimeStamp(
      const GxTimeStampPtr& element);

  virtual void VisitGxTour(
      const GxTourPtr& element);

  virtual void VisitGxTourControl(
      const GxTourControlPtr& element);

  virtual void VisitGxTourPrimitive(
      const GxTourPrimitivePtr& element);

  virtual void VisitGxTrack(
      const GxTrackPtr& element);

  virtual void VisitGxWait(
      const GxWaitPtr& element);

  virtual void VisitHotSpot(
      const HotSpotPtr& element);

  virtual void VisitIcon(
      const IconPtr& element);

  virtual void VisitIconStyle(
      const IconStylePtr& element);

  virtual void VisitIconStyleIcon(
      const IconStyleIconPtr& element);

  virtual void VisitImagePyramid(
      const ImagePyramidPtr& element);

  virtual void VisitInnerBoundaryIs(
      const InnerBoundaryIsPtr& element);

  virtual void VisitItemIcon(
      const ItemIconPtr& element);

  virtual void VisitKml(
      const KmlPtr& element);

  virtual void VisitLabelStyle(
      const LabelStylePtr& element);

  virtual void VisitLatLonAltBox(
      const LatLonAltBoxPtr& element);

  virtual void VisitLatLonBox(
      const LatLonBoxPtr& element);

  virtual void VisitLineString(
      const LineStringPtr& element);

  virtual void VisitLineStyle(
      const LineStylePtr& element);

  virtual void VisitLinearRing(
      const LinearRingPtr& element);

  virtual void VisitLink(
      const LinkPtr& element);

  virtual void VisitLinkSnippet(
      const LinkSnippetPtr& element);

  virtual void VisitListStyle(
      const ListStylePtr& element);

  virtual void VisitLocation(
      const LocationPtr& element);

  virtual void VisitLod(
      const LodPtr& element);

  virtual void VisitLookAt(
      const LookAtPtr& element);

  virtual void VisitMetadata(
      const MetadataPtr& element);

  virtual void VisitModel(
      const ModelPtr& element);

  virtual void VisitMultiGeometry(
      const MultiGeometryPtr& element);

  virtual void VisitNetworkLink(
      const NetworkLinkPtr& element);

  virtual void VisitNetworkLinkControl(
      const NetworkLinkControlPtr& element);

  virtual void VisitObject(
      const ObjectPtr& element);

  virtual void VisitOrientation(
      const OrientationPtr& element);

  virtual void VisitOuterBoundaryIs(
      const OuterBoundaryIsPtr& element);

  virtual void VisitOverlay(
      const OverlayPtr& element);

  virtual void VisitOverlayXY(
      const OverlayXYPtr& element);

  virtual void VisitPair(
      const PairPtr& element);

  virtual void VisitPhotoOverlay(
      const PhotoOverlayPtr& element);

  virtual void VisitPlacemark(
      const PlacemarkPtr& element);

  virtual void VisitPoint(
      const PointPtr& element);

  virtual void VisitPolyStyle(
      const PolyStylePtr& element);

  virtual void VisitPolygon(
      const PolygonPtr& element);

  virtual void VisitRegion(
      const RegionPtr& element);

  virtual void VisitResourceMap(
      const ResourceMapPtr& element);

  virtual void VisitRotationXY(
      const RotationXYPtr& element);

  virtual void VisitScale(
      const ScalePtr& element);

  virtual void VisitSchema(
      const SchemaPtr& element);

  virtual void VisitSchemaData(
      const SchemaDataPtr& element);

  virtual void VisitScreenOverlay(
      const ScreenOverlayPtr& element);

  virtual void VisitScreenXY(
      const ScreenXYPtr& element);

  virtual void VisitSimpleData(
      const SimpleDataPtr& element);

  virtual void VisitSimpleField(
      const SimpleFieldPtr& element);

  virtual void VisitSize(
      const SizePtr& element);

  virtual void VisitSnippet(
      const SnippetPtr& element);

  virtual void VisitStyle(
      const StylePtr& element);

  virtual void VisitStyleMap(
      const StyleMapPtr& element);

  virtual void VisitStyleSelector(
      const StyleSelectorPtr& element);

  virtual void VisitSubStyle(
      const SubStylePtr& element);

  virtual void VisitTimePrimitive(
      const TimePrimitivePtr& element);

  virtual void VisitTimeSpan(
      const TimeSpanPtr& element);

  virtual void VisitTimeStamp(
      const TimeStampPtr& element);

  virtual void VisitUpdate(
      const UpdatePtr& element);

  virtual void VisitUpdateOperation(
      const UpdateOperationPtr& element);

  virtual void VisitUrl(
      const UrlPtr& element);

  virtual void VisitVec2(
      const Vec2Ptr& element);

  virtual void VisitViewVolume(
      const ViewVolumePtr& element);

  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Visitor);
};

}  // end namespace kmldom

#endif  // KML_DOM_VISITOR_H__
