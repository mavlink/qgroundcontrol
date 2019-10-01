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

// This file contains the declarations of location-related utilities.
// Since the location of a Feature or Geometry is closely tied to KML
// these functions are considered part of the KML Engine and not
// simply pure convenenience.

#ifndef KML_ENGINE_LOCATION_UTIL_H__
#define KML_ENGINE_LOCATION_UTIL_H__

#include "kml/dom.h"

namespace kmlengine {

class Bbox;

// This returns the latitude half way between the north and south and
// the longitude half way between the east and west of the given LatLonBox
// or LatLonAltBox.
void GetCenter(const kmldom::AbstractLatLonBoxPtr& allb,
               double* lat, double* lon);

// This returns the n,s,e,w bounds of the given list of coordinates.  This
// returns true if the coordinates are not empty.  A NULL bbox is ignored.
bool GetCoordinatesBounds(const kmldom::CoordinatesPtr& coordinates,
                          Bbox* bbox);

// This returns the n,s,e,w bounds of the given Feature.  If the Feature is
// a Container this is the bounds of all Features within that Container
// recursively.  This returns true if the coordinates are not empty.
// A NULL bbox is ignored.
bool GetFeatureBounds(const kmldom::FeaturePtr& placemark, Bbox* bbox);

// Return the location of the Feature.
bool GetFeatureLatLon(const kmldom::FeaturePtr& placemark,
                      double* lat, double* lon);

// This returns the bounding box of any Geometry including: Point, LineString,
// LinearRing, Polygon, Model, and MultiGeometry.  If the Geometry has no
// location (empty or missing <coordinates>, for example) false is returned.
// A NULL bbox is ignored and does not affect the return value.
bool GetGeometryBounds(const kmldom::GeometryPtr& geometry, Bbox* bbox);

// Return the location of the Geometry.
bool GetGeometryLatLon(const kmldom::GeometryPtr& geometry,
                      double* lat, double* lon);

// This returns the bounds of the coordinates child of the given element.
// CP can be one of LineString, LinearRing or Point.
template<class CP>
bool GetCoordinatesParentBounds(const CP& cp, Bbox* bbox) {
  return cp && cp->has_coordinates() &&
      GetCoordinatesBounds(cp->get_coordinates(), bbox);
}

// Return the bounds of the Model.
bool GetModelBounds(const kmldom::ModelPtr& model, Bbox* bbox);

// Return the location of the Model.
bool GetModelLatLon(const kmldom::ModelPtr& model, double* lat, double* lon);

// Return the location of the Placemark.
bool GetPlacemarkLatLon(const kmldom::PlacemarkPtr& placemark,
                        double* lat, double* lon);

// Return the location of the Point.
bool GetPointLatLon(const kmldom::PointPtr& point, double* lat, double* lon);

}  // end namespace kmlengine

#endif  // KML_ENGINE_LOCATION_UTIL_H__
