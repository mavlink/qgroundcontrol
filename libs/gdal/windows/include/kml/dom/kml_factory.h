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

#ifndef KML_DOM_KML_FACTORY_H__
#define KML_DOM_KML_FACTORY_H__

#include <string>
#include "kml/dom/kmldom.h"
#include "kml/base/util.h"

namespace kmldom {

// A singleton factory class.
class KmlFactory {
 public:
  static KmlFactory* GetFactory();

  // Factory functions used by the parser to create any KML element.
  ElementPtr CreateElementById(KmlDomType id) const;
  ElementPtr CreateElementFromName(const string& element_name) const;
  Field* CreateFieldById(KmlDomType type_id) const;

  // Factory functions to create all KML complex elements.
  Alias* CreateAlias() const;
  AtomAuthor* CreateAtomAuthor() const;
  AtomCategory* CreateAtomCategory() const;
  AtomContent* CreateAtomContent() const;
  AtomEntry* CreateAtomEntry() const;
  AtomFeed* CreateAtomFeed() const;
  AtomLink* CreateAtomLink() const;
  BalloonStyle* CreateBalloonStyle() const;
  Coordinates* CreateCoordinates() const;
  Camera* CreateCamera() const;
  Change* CreateChange() const;
  Create* CreateCreate() const;
  Data* CreateData() const;
  Delete* CreateDelete() const;
  Document* CreateDocument() const;
  ExtendedData* CreateExtendedData() const;
  Folder* CreateFolder() const;
  GroundOverlay* CreateGroundOverlay() const;
  HotSpot* CreateHotSpot() const;
  Icon* CreateIcon() const;
  IconStyle* CreateIconStyle() const;
  IconStyleIcon* CreateIconStyleIcon() const;
  ImagePyramid* CreateImagePyramid() const;
  InnerBoundaryIs* CreateInnerBoundaryIs() const;
  ItemIcon* CreateItemIcon() const;
  Kml* CreateKml() const;
  LabelStyle* CreateLabelStyle() const;
  LatLonBox* CreateLatLonBox() const;
  LatLonAltBox* CreateLatLonAltBox() const;
  LinearRing* CreateLinearRing() const;
  LineString* CreateLineString() const;
  LineStyle* CreateLineStyle() const;
  Link* CreateLink() const;
  LinkSnippet* CreateLinkSnippet() const;
  ListStyle* CreateListStyle() const;
  Location* CreateLocation() const;
  Lod* CreateLod() const;
  LookAt* CreateLookAt() const;
  Metadata* CreateMetadata() const;
  Model* CreateModel() const;
  MultiGeometry* CreateMultiGeometry() const;
  NetworkLink* CreateNetworkLink() const;
  Orientation* CreateOrientation() const;
  NetworkLinkControl* CreateNetworkLinkControl() const;
  OuterBoundaryIs* CreateOuterBoundaryIs() const;
  OverlayXY* CreateOverlayXY() const;
  Pair* CreatePair() const;
  PhotoOverlay* CreatePhotoOverlay() const;
  Placemark* CreatePlacemark() const;
  Polygon* CreatePolygon() const;
  Point* CreatePoint() const;
  PolyStyle* CreatePolyStyle() const;
  Region* CreateRegion() const;
  ResourceMap* CreateResourceMap() const;
  RotationXY* CreateRotationXY() const;
  Scale* CreateScale() const;
  Schema* CreateSchema() const;
  SchemaData* CreateSchemaData() const;
  ScreenOverlay* CreateScreenOverlay() const;
  ScreenXY* CreateScreenXY() const;
  Size* CreateSize() const;
  SimpleData* CreateSimpleData() const;
  SimpleField* CreateSimpleField() const;
  Snippet* CreateSnippet() const;
  Style* CreateStyle() const;
  StyleMap* CreateStyleMap() const;
  TimeSpan* CreateTimeSpan() const;
  TimeStamp* CreateTimeStamp() const;
  ViewVolume* CreateViewVolume() const;
  Update* CreateUpdate() const;
  Url* CreateUrl() const;
  XalAddressDetails* CreateXalAddressDetails() const;
  XalAdministrativeArea* CreateXalAdministrativeArea() const;
  XalCountry* CreateXalCountry() const;
  XalLocality* CreateXalLocality() const;
  XalPostalCode* CreateXalPostalCode() const;
  XalSubAdministrativeArea* CreateXalSubAdministrativeArea() const;
  XalThoroughfare* CreateXalThoroughfare() const;

  // These methods create the elements in the Google extensions to KML 2.2.
  GxAnimatedUpdate* CreateGxAnimatedUpdate() const;
  GxFlyTo* CreateGxFlyTo() const;
  GxLatLonQuad* CreateGxLatLonQuad() const;
  GxMultiTrack* CreateGxMultiTrack() const;
  GxPlaylist* CreateGxPlaylist() const;
  GxSimpleArrayData* CreateGxSimpleArrayData() const;
  GxSimpleArrayField* CreateGxSimpleArrayField() const;
  GxSoundCue* CreateGxSoundCue() const;
  GxTimeSpan* CreateGxTimeSpan() const;
  GxTimeStamp* CreateGxTimeStamp() const;
  GxTour* CreateGxTour() const;
  GxTourControl* CreateGxTourControl() const;
  GxTrack* CreateGxTrack() const;
  GxWait* CreateGxWait() const;

 private:
  KmlFactory() {};  // Singleton class, use GetFactory().
  static KmlFactory* factory_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(KmlFactory);
};

}  // namespace kmldom

#endif  // KML_DOM_KML_FACTORY_H__
