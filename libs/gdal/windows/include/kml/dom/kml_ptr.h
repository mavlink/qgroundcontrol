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

#ifndef KML_DOM_KML_PTR_H__
#define KML_DOM_KML_PTR_H__

#include "boost/intrusive_ptr.hpp"

namespace kmldom {

class Element;
class Field;

class AbstractLatLonBox;
class AbstractLink;
class AbstractView;
class BasicLink;
class ColorStyle;
class Container;
class Feature;
class Geometry;
class Object;
class Overlay;
class StyleSelector;
class SubStyle;
class TimePrimitive;
class Vec2;

class Alias;
class AtomAuthor;
class AtomCategory;
class AtomContent;
class AtomEntry;
class AtomFeed;
class AtomLink;
class BalloonStyle;
class Camera;
class Change;
class Coordinates;
class Create;
class Data;
class Delete;
class Document;
class ExtendedData;
class Folder;
class GroundOverlay;
class Icon;
class IconStyle;
class IconStyleIcon;
class ImagePyramid;
class ItemIcon;
class LabelStyle;
class LatLonAltBox;
class LatLonBox;
class LineString;
class LineStyle;
class LinearRing;
class Link;
class ListStyle;
class Location;
class Lod;
class LookAt;
class Metadata;
class Model;
class MultiGeometry;
class NetworkLink;
class NetworkLinkControl;
class Orientation;
class Pair;
class PhotoOverlay;
class Placemark;
class Point;
class PolyStyle;
class Polygon;
class Region;
class ResourceMap;
class Scale;
class Schema;
class SchemaData;
class ScreenOverlay;
class SimpleData;
class SimpleField;
class Snippet;
class Style;
class StyleMap;
class TimeSpan;
class TimeStamp;
class Update;
class UpdateOperation;
class Url;
class ViewVolume;
class HotSpot;
class InnerBoundaryIs;
class Kml;
class LinkSnippet;
class OuterBoundaryIs;
class OverlayXY;
class RotationXY;
class ScreenXY;
class Size;

class XalAddressDetails;
class XalAdministrativeArea;
class XalCountry;
class XalLocality;
class XalPostalCode;
class XalSubAdministrativeArea;
class XalThoroughfare;

class GxAnimatedUpdate;
class GxFlyTo;
class GxLatLonQuad;
class GxMultiTrack;
class GxPlaylist;
class GxSimpleArrayField;
class GxSimpleArrayData;
class GxSoundCue;
class GxTimeSpan;
class GxTimeStamp;
class GxTimePrimitive;
class GxTour;
class GxTourControl;
class GxTourPrimitive;
class GxTrack;
class GxWait;

typedef boost::intrusive_ptr<Element> ElementPtr;
typedef boost::intrusive_ptr<Field> FieldPtr;

typedef boost::intrusive_ptr<AbstractLatLonBox> AbstractLatLonBoxPtr;
typedef boost::intrusive_ptr<AbstractLink> AbstractLinkPtr;
typedef boost::intrusive_ptr<AbstractView> AbstractViewPtr;
typedef boost::intrusive_ptr<BasicLink> BasicLinkPtr;
typedef boost::intrusive_ptr<ColorStyle> ColorStylePtr;
typedef boost::intrusive_ptr<Container> ContainerPtr;
typedef boost::intrusive_ptr<Feature> FeaturePtr;
typedef boost::intrusive_ptr<Geometry> GeometryPtr;
typedef boost::intrusive_ptr<Object> ObjectPtr;
typedef boost::intrusive_ptr<Overlay> OverlayPtr;
typedef boost::intrusive_ptr<StyleSelector> StyleSelectorPtr;
typedef boost::intrusive_ptr<SubStyle> SubStylePtr;
typedef boost::intrusive_ptr<TimePrimitive> TimePrimitivePtr;
typedef boost::intrusive_ptr<Vec2> Vec2Ptr;

typedef boost::intrusive_ptr<Alias> AliasPtr;
typedef boost::intrusive_ptr<AtomAuthor> AtomAuthorPtr;
typedef boost::intrusive_ptr<AtomCategory> AtomCategoryPtr;
typedef boost::intrusive_ptr<AtomContent> AtomContentPtr;
typedef boost::intrusive_ptr<AtomEntry> AtomEntryPtr;
typedef boost::intrusive_ptr<AtomFeed> AtomFeedPtr;
typedef boost::intrusive_ptr<AtomLink> AtomLinkPtr;
typedef boost::intrusive_ptr<BalloonStyle> BalloonStylePtr;
typedef boost::intrusive_ptr<Camera> CameraPtr;
typedef boost::intrusive_ptr<Change> ChangePtr;
typedef boost::intrusive_ptr<Coordinates> CoordinatesPtr;
typedef boost::intrusive_ptr<Create> CreatePtr;
typedef boost::intrusive_ptr<Data> DataPtr;
typedef boost::intrusive_ptr<Delete> DeletePtr;
typedef boost::intrusive_ptr<Document> DocumentPtr;
typedef boost::intrusive_ptr<ExtendedData> ExtendedDataPtr;
typedef boost::intrusive_ptr<Folder> FolderPtr;
typedef boost::intrusive_ptr<GroundOverlay> GroundOverlayPtr;
typedef boost::intrusive_ptr<Icon> IconPtr;
typedef boost::intrusive_ptr<IconStyle> IconStylePtr;
typedef boost::intrusive_ptr<IconStyleIcon> IconStyleIconPtr;
typedef boost::intrusive_ptr<ImagePyramid> ImagePyramidPtr;
typedef boost::intrusive_ptr<ItemIcon> ItemIconPtr;
typedef boost::intrusive_ptr<LabelStyle> LabelStylePtr;
typedef boost::intrusive_ptr<LatLonAltBox> LatLonAltBoxPtr;
typedef boost::intrusive_ptr<LatLonBox> LatLonBoxPtr;
typedef boost::intrusive_ptr<LineString> LineStringPtr;
typedef boost::intrusive_ptr<LineStyle> LineStylePtr;
typedef boost::intrusive_ptr<LinearRing> LinearRingPtr;
typedef boost::intrusive_ptr<Link> LinkPtr;
typedef boost::intrusive_ptr<ListStyle> ListStylePtr;
typedef boost::intrusive_ptr<Location> LocationPtr;
typedef boost::intrusive_ptr<Lod> LodPtr;
typedef boost::intrusive_ptr<LookAt> LookAtPtr;
typedef boost::intrusive_ptr<Metadata> MetadataPtr;
typedef boost::intrusive_ptr<Model> ModelPtr;
typedef boost::intrusive_ptr<MultiGeometry> MultiGeometryPtr;
typedef boost::intrusive_ptr<NetworkLink> NetworkLinkPtr;
typedef boost::intrusive_ptr<NetworkLinkControl> NetworkLinkControlPtr;
typedef boost::intrusive_ptr<Orientation> OrientationPtr;
typedef boost::intrusive_ptr<Pair> PairPtr;
typedef boost::intrusive_ptr<PhotoOverlay> PhotoOverlayPtr;
typedef boost::intrusive_ptr<Placemark> PlacemarkPtr;
typedef boost::intrusive_ptr<Point> PointPtr;
typedef boost::intrusive_ptr<PolyStyle> PolyStylePtr;
typedef boost::intrusive_ptr<Polygon> PolygonPtr;
typedef boost::intrusive_ptr<Region> RegionPtr;
typedef boost::intrusive_ptr<ResourceMap> ResourceMapPtr;
typedef boost::intrusive_ptr<Scale> ScalePtr;
typedef boost::intrusive_ptr<Schema> SchemaPtr;
typedef boost::intrusive_ptr<SchemaData> SchemaDataPtr;
typedef boost::intrusive_ptr<ScreenOverlay> ScreenOverlayPtr;
typedef boost::intrusive_ptr<SimpleData> SimpleDataPtr;
typedef boost::intrusive_ptr<SimpleField> SimpleFieldPtr;
typedef boost::intrusive_ptr<Snippet> SnippetPtr;
typedef boost::intrusive_ptr<Style> StylePtr;
typedef boost::intrusive_ptr<StyleMap> StyleMapPtr;
typedef boost::intrusive_ptr<TimeSpan> TimeSpanPtr;
typedef boost::intrusive_ptr<TimeStamp> TimeStampPtr;
typedef boost::intrusive_ptr<Update> UpdatePtr;
typedef boost::intrusive_ptr<UpdateOperation> UpdateOperationPtr;
typedef boost::intrusive_ptr<Url> UrlPtr;
typedef boost::intrusive_ptr<ViewVolume> ViewVolumePtr;
typedef boost::intrusive_ptr<HotSpot> HotSpotPtr;
typedef boost::intrusive_ptr<InnerBoundaryIs> InnerBoundaryIsPtr;
typedef boost::intrusive_ptr<Kml> KmlPtr;
typedef boost::intrusive_ptr<LinkSnippet> LinkSnippetPtr;
typedef boost::intrusive_ptr<OuterBoundaryIs> OuterBoundaryIsPtr;
typedef boost::intrusive_ptr<OverlayXY> OverlayXYPtr;
typedef boost::intrusive_ptr<RotationXY> RotationXYPtr;
typedef boost::intrusive_ptr<ScreenXY> ScreenXYPtr;
typedef boost::intrusive_ptr<Size> SizePtr;

typedef boost::intrusive_ptr<XalAddressDetails> XalAddressDetailsPtr;
typedef boost::intrusive_ptr<XalAdministrativeArea> XalAdministrativeAreaPtr;
typedef boost::intrusive_ptr<XalCountry> XalCountryPtr;
typedef boost::intrusive_ptr<XalLocality> XalLocalityPtr;
typedef boost::intrusive_ptr<XalPostalCode> XalPostalCodePtr;
typedef boost::intrusive_ptr<XalSubAdministrativeArea>
   XalSubAdministrativeAreaPtr;
typedef boost::intrusive_ptr<XalThoroughfare> XalThoroughfarePtr;

typedef boost::intrusive_ptr<GxAnimatedUpdate> GxAnimatedUpdatePtr;
typedef boost::intrusive_ptr<GxFlyTo> GxFlyToPtr;
typedef boost::intrusive_ptr<GxLatLonQuad> GxLatLonQuadPtr;
typedef boost::intrusive_ptr<GxMultiTrack> GxMultiTrackPtr;
typedef boost::intrusive_ptr<GxPlaylist> GxPlaylistPtr;
typedef boost::intrusive_ptr<GxSimpleArrayField> GxSimpleArrayFieldPtr;
typedef boost::intrusive_ptr<GxSimpleArrayData> GxSimpleArrayDataPtr;
typedef boost::intrusive_ptr<GxSoundCue> GxSoundCuePtr;
typedef boost::intrusive_ptr<GxTimeSpan> GxTimeSpanPtr;
typedef boost::intrusive_ptr<GxTimeStamp> GxTimeStampPtr;
typedef boost::intrusive_ptr<GxTour> GxTourPtr;
typedef boost::intrusive_ptr<GxTourControl> GxTourControlPtr;
typedef boost::intrusive_ptr<GxTourPrimitive> GxTourPrimitivePtr;
typedef boost::intrusive_ptr<GxTrack> GxTrackPtr;
typedef boost::intrusive_ptr<GxWait> GxWaitPtr;

}  // end namespace kmldom

#endif  // KML_DOM_KML_PTR_H__
