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

// This file contains the declarations of some mathematical functions useful
// when working with KML or, more generally, geometry on a Great Circle.
//
// Many of the formulae here were cribbed from the excellent Aviation
// Formulary: http://williams.best.vwh.net/avform.htm
//
// NOTE: with the exception of the functions explicitly for converting between
// units, all functions here accept and return coordinates and angles in
// decimal degrees, and distances in meters.

#ifndef KML_BASE_MATH_UTIL_H__
#define KML_BASE_MATH_UTIL_H__

#include <math.h>
// At least one variant of MSVC does not define M_PI.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <utility>
#include "kml/base/vec3.h"

namespace kmlbase {

// Returns the initial azimuth (the angle measured clockwise from true north)
// at a point from that point to a second point. For example, the azimuth
// from 0,0 to 1,0 is 0 degrees. From 0,0 to 0,1 is 90 degrees (due east).
// The range of the result is (-180, 180].
//
// This is directly useful as the value of <heading> in KML's AbstractView.
//
// Note that this is the _initial_ azimuth; it changes as one follows the
// great circle path from point 1 to point2.
double AzimuthBetweenPoints(double lat_from, double lng_from,
                            double lat_to, double lng_to);

// Returns the angle from the horizontal plane between alt1 and alt2.
// For example, the returned angle from (37.00, -121.98, 600) to a point
// about 1778 meters west, 400 meters below at (37.00, -122.00, 200) is
// -12.7 degrees.
//
// To use this as the value of KML's <tilt>, add 90 degrees (since in KML a
// tilt of 0 is vertical.
//
// TODO: this is a naive implementation accurate only over short distances.
// It does not yet account for surface curvature.
double ElevationBetweenPoints(double lat_from, double lng_from, double alt_from,
                              double lat_to, double lng_to, double alt_to);

// Returns the great circle distance in meters between two points on the
// Earth's surface. The antemeridian is not considered here.
double DistanceBetweenPoints(double lat_from, double lng_from,
                             double lat_to, double lng_to);

// Returns the great circle distance in meters between two 3d points. The
// antemeridian is not considered here.
double DistanceBetweenPoints3d(
    double lat_from, double lng_from, double alt_from,
    double lat_to, double lng_to, double alt_to);

// Given a vector describing a line at an angle from the horizontal plane,
// where the vector starts at a point on the surface of the Earth,
// returns the absolute distance between the ground point and the point
// directly under the end point.
double GroundDistanceFromRangeAndElevation(double range, double elevation);

// Given a vector describing a line at an angle from the horizontal plane,
// where the vector starts at a point on the surface of the Earth,
// returns the absolute height between the end point and the surface
// point directly under it.
double HeightFromRangeAndElevation(double range, double elevation);

// Returns a Vec3 containing the latitude and longitude of a point at a
// distance (meters) out on the radial (degrees) from a center point lat, lng.
// The radial is measured clockwise from north. The antemeridian is not
// considered here.
Vec3 LatLngOnRadialFromPoint(double lat, double lng,
                             double distance, double radial);

// These functions are mostly internal, used in converting between degrees and
// radians.
double DegToRad(double degrees);
double RadToDeg(double radians);
double MetersToRadians(double meters);
double RadiansToMeters(double radians);

}  // end namespace kmlbase

#endif  // KML_BASE_MATH_UTIL_H__
