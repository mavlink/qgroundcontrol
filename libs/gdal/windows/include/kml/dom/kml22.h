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

// This file defines the id of each element.

// The element names exactly match the OGC KML 2.2 Standard:
// http://www.opengeospatial.org/standards/kml
// See also Google's reference especially for the "gx" elements:
// http://code.google.com/apis/kml/documentation/kmlreference.html
// Type_${element_name}

#ifndef KML_DOM_KML22_H__
#define KML_DOM_KML22_H__

namespace kmldom {

// This list matches kml22_elements_[] exactly:
typedef enum {
  Type_Unknown,

  Type_ColorStyle,
  Type_Container,
  Type_Feature,
  Type_Geometry,
  Type_AbstractLatLonBox,
  Type_Object,
  Type_Overlay,
  Type_StyleSelector,
  Type_SubStyle,
  Type_TimePrimitive,
  Type_AbstractView,
  Type_BasicLink,
  Type_Vec2,

  Type_Alias,
  Type_BalloonStyle,
  Type_Camera,
  Type_Change,
  Type_Create,
  Type_Data,
  Type_Delete,
  Type_Document,
  Type_ExtendedData,
  Type_Folder,
  Type_GroundOverlay,
  Type_Icon,
  Type_IconStyle,
  Type_IconStyleIcon,
  Type_ImagePyramid,
  Type_ItemIcon,
  Type_LabelStyle,
  Type_LatLonAltBox,
  Type_LatLonBox,
  Type_LineString,
  Type_LineStyle,
  Type_LinearRing,
  Type_Link,
  Type_ListStyle,
  Type_Location,
  Type_Lod,
  Type_LookAt,
  Type_Metadata,
  Type_Model,
  Type_MultiGeometry,
  Type_NetworkLink,
  Type_NetworkLinkControl,
  Type_Orientation,
  Type_Pair,
  Type_PhotoOverlay,
  Type_Placemark,
  Type_Point,
  Type_PolyStyle,
  Type_Polygon,
  Type_Region,
  Type_ResourceMap,
  Type_Scale,
  Type_Schema,
  Type_SchemaData,
  Type_ScreenOverlay,
  Type_SimpleData,
  Type_SimpleField,
  Type_Snippet,
  Type_Style,
  Type_StyleMap,
  Type_TimeSpan,
  Type_TimeStamp,
  Type_Update,
  Type_Url,
  Type_ViewVolume,

  Type_address,
  Type_altitude,
  Type_altitudeMode,
  Type_altitudeModeGroup,
  Type_begin,
  Type_bgColor,
  Type_bottomFov,
  Type_color,
  Type_colorMode,
  Type_cookie,
  Type_coordinates,
  Type_description,
  Type_displayMode,
  Type_displayName,
  Type_drawOrder,
  Type_east,
  Type_end,
  Type_expires,
  Type_extrude,
  Type_fill,
  Type_flyToView,
  Type_gridOrigin,
  Type_heading,
  Type_hotSpot,
  Type_href,
  Type_httpQuery,
  Type_innerBoundaryIs,
  Type_key,
  Type_kml,
  Type_latitude,
  Type_leftFov,
  Type_linkDescription,
  Type_linkName,
  Type_linkSnippet,
  Type_listItemType,
  Type_longitude,
  Type_maxAltitude,
  Type_maxFadeExtent,
  Type_maxHeight,
  Type_maxLength,
  Type_maxLodPixels,
  Type_maxSessionLength,
  Type_maxSnippetLines,
  Type_maxWidth,
  Type_message,
  Type_minAltitude,
  Type_minFadeExtent,
  Type_minLodPixels,
  Type_minRefreshPeriod,
  Type_name,
  Type_near,
  Type_north,
  Type_open,
  Type_outerBoundaryIs,
  Type_outline,
  Type_overlayXY,
  Type_phoneNumber,
  Type_range,
  Type_refreshInterval,
  Type_refreshMode,
  Type_refreshVisibility,
  Type_rightFov,
  Type_roll,
  Type_rotation,
  Type_rotationXY,
  Type_scale,
  Type_screenXY,
  Type_shape,
  Type_size,
  Type_snippet,
  Type_sourceHref,
  Type_south,
  Type_state,
  Type_styleUrl,
  Type_targetHref,
  Type_tessellate,
  Type_text,
  Type_textColor,
  Type_tileSize,
  Type_tilt,
  Type_topFov,
  Type_units,
  Type_value,
  Type_viewBoundScale,
  Type_viewFormat,
  Type_viewRefreshMode,
  Type_viewRefreshTime,
  Type_visibility,
  Type_west,
  Type_when,
  Type_width,
  Type_x,
  Type_y,
  Type_z,

  Type_AtomAuthor,
  Type_AtomCategory,
  Type_AtomContent,
  Type_AtomEntry,
  Type_AtomFeed,
  Type_AtomLink,

  Type_atomEmail,
  Type_atomId,
  Type_atomLabel,
  Type_atomName,
  Type_atomScheme,
  Type_atomSummary,
  Type_atomTerm,
  Type_atomTitle,
  Type_atomUpdated,
  Type_atomUri,

  Type_XalAddressDetails,
  Type_XalAdministrativeArea,
  Type_XalCountry,
  Type_XalLocality,
  Type_XalPostalCode,
  Type_XalSubAdministrativeArea,
  Type_XalThoroughfare,

  Type_xalAdministrativeAreaName,
  Type_xalCountryNameCode,
  Type_xalLocalityName,
  Type_xalPostalCodeNumber,
  Type_xalSubAdministrativeAreaName,
  Type_xalThoroughfareName,
  Type_xalThoroughfareNumber,

  Type_GxTourPrimitive,

  Type_GxAnimatedUpdate,
  Type_GxFlyTo,
  Type_GxLatLonQuad,
  Type_GxMultiTrack,
  Type_GxPlaylist,
  Type_GxSimpleArrayData,
  Type_GxSimpleArrayField,
  Type_GxSoundCue,
  Type_GxTimeSpan,
  Type_GxTimeStamp,
  Type_GxTour,
  Type_GxTourControl,
  Type_GxTrack,
  Type_GxWait,

  Type_GxAltitudeMode,
  Type_GxAngles,
  Type_GxBalloonVisibility,
  Type_GxCoord,
  Type_GxDuration,
  Type_GxFlyToMode,
  Type_GxH,
  Type_GxInterpolate,
  Type_GxPlayMode,
  Type_GxValue,
  Type_GxW,
  Type_GxX,
  Type_GxY,

  Type_Invalid
} KmlDomType;

// The value of each enum is the offset to the corresponding string in the
// kKml22Enums table.  The enum type name here is the element name with first
// char folded up to be consistent with the convention of type names starting
// with an upper case letter, hence <altitudeMode>'s enum values are of type
// AltitudeModeEnum.  Each enum value repeats the name of the element with all
// chars folded up followed by an underscore followed by the name of the
// enumeration value folded to upper case.  Thus DOM API code examining a
// <LookAt>'s <altitudeMode> might be as follows:
//
//  AltitudeModeEnum altitudemode = lookat->altitudemode();
//  switch (altitudemode) {
//    case ALTITUDEMODE_CLAMPTOGROUND:
//      ...
//    case ALTITUDEMODE_RELATIVETOGROUND:
//      ...
//    case ALTITUDEMODE_ABSOLUTE
//      ...
//    default:
//      // unknown altitudeMode
//  };

typedef enum {
  ALTITUDEMODE_CLAMPTOGROUND = 0,
  ALTITUDEMODE_RELATIVETOGROUND,
  ALTITUDEMODE_ABSOLUTE
} AltitudeModeEnum;

typedef enum {
  COLORMODE_NORMAL = 0,
  COLORMODE_RANDOM
} ColorModeEnum;

typedef enum {
  DISPLAYMODE_DEFAULT = 0,
  DISPLAYMODE_HIDE
} DisplayModeEnum;

typedef enum {
  GRIDORIGIN_LOWERLEFT = 0,
  GRIDORIGIN_UPPERLEFT
} GridOriginEnum;

typedef enum {
  ITEMICONSTATE_OPEN = 0,
  ITEMICONSTATE_CLOSED,
  ITEMICONSTATE_ERROR,
  ITEMICONSTATE_FETCHING0,
  ITEMICONSTATE_FETCHING1,
  ITEMICONSTATE_FETCHING2
} ItemIconStateEnum;

typedef enum {
  LISTITEMTYPE_CHECK = 0,
  LISTITEMTYPE_RADIOFOLDER,
  LISTITEMTYPE_CHECKOFFONLY,
  LISTITEMTYPE_CHECKHIDECHILDREN
} ListItemTypeEnum;

typedef enum {
  REFRESHMODE_ONCHANGE = 0,
  REFRESHMODE_ONINTERVAL,
  REFRESHMODE_ONEXPIRE
} RefreshModeEnum;

typedef enum {
  SHAPE_RECTANGLE = 0,
  SHAPE_CYLINDER,
  SHAPE_SPHERE
} ShapeEnum;

typedef enum {
  STYLESTATE_NORMAL = 0,
  STYLESTATE_HIGHLIGHT
} StyleStateEnum;

typedef enum {
  UNITS_FRACTION = 0,
  UNITS_PIXELS,
  UNITS_INSETPIXELS
} UnitsEnum;

typedef enum {
  VIEWREFRESHMODE_NEVER = 0,
  VIEWREFRESHMODE_ONREQUEST,
  VIEWREFRESHMODE_ONSTOP,
  VIEWREFRESHMODE_ONREGION
} ViewRefreshModeEnum;

typedef enum {
  GX_ALTITUDEMODE_CLAMPTOSEAFLOOR = 0,
  GX_ALTITUDEMODE_RELATIVETOSEAFLOOR
} GxAltitudeModeEnum;

typedef enum {
  GX_FLYTOMODE_BOUNCE = 0,
  GX_FLYTOMODE_SMOOTH
} GxFlyToModeEnum;

typedef enum {
  GX_PLAYMODE_PAUSE = 0
} GxPlayModeEnum;

}  // end namespace kmldom

#endif  // KML_DOM_KML22_H__
