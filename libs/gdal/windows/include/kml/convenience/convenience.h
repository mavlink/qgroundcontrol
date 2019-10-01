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

// This file contains the declarations of some KML convenience functions.

#ifndef KML_CONVENIENCE_CONVENIENCE_H__
#define KML_CONVENIENCE_CONVENIENCE_H__

#include <vector>
#include "kml/base/vec3.h"
#include "kml/dom.h"

namespace kmlbase {
class DateTime;
}

namespace kmlconvenience {

// NOTE: this collection of convenience routines is expected to grow.
// NOTE: for now these are all in one file in alphabetical order.

// This creates a Data element with the given name and value and appends
// this to the Feature's ExtendedData.  An ExtendedData is created in the
// Feature if one does not already exist.
void AddExtendedDataValue(const string& name, const string& value,
                          kmldom::FeaturePtr feature);

// Creates a <gx:AnimatedUpdate> with a <Change> to a Point Placemark of
// the specified target_id and coordinates as specified by vec3.
kmldom::GxAnimatedUpdatePtr CreateAnimatedUpdateChangePoint(
    const string& target_id, const kmlbase::Vec3& vec3, double duration);

// Creates a simple Polygon Placemark from a LinearRing.
kmldom::PlacemarkPtr CreateBasicPolygonPlacemark(
    const kmldom::LinearRingPtr& lr);

// Creates a <Camera> element with the specified fields.
kmldom::CameraPtr CreateCamera(double latitude, double longitude,
                               double altitude, double heading, double tilt,
                               double roll, int altitudemode);

// Creates a <coordinates> element filled with the lng, lat[, alt] tuples
// describing a great circle of radius around a point lat, lng. The
// antemeridian is not considered here.
kmldom::CoordinatesPtr CreateCoordinatesCircle(double lat, double lng,
                                               double radius, size_t segments);

// This creates a Data element with the name and value specified:
// <Data name="NAME><value>VALUE</value></Data>
kmldom::DataPtr CreateDataNameValue(const string& name,
                                    const string& value);

// Creates a <LookAt> element from the specified fields.
kmldom::LookAtPtr CreateLookAt(double latitude, double longitude,
                               double altitude, double heading, double tilt,
                               double range, int altitudemode);

// If the atts contains both a double "lat" and double "lon" then create
// a KML <Point> with <coordinates> set from these attributes.
kmldom::PointPtr CreatePointFromLatLonAtts(const char** atts);

// Create a <Point> with <coordinates> from the given Vec3.
kmldom::PointPtr CreatePointFromVec3(const kmlbase::Vec3& vec);

// This creates a Point coordinates set as indicated.
kmldom::PointPtr CreatePointLatLon(double lat, double lon);

// This is a convenience function to create a Point Placemark.
kmldom::PlacemarkPtr CreatePointPlacemark(const string& name,
                                          double lat, double lon);

// Create a <Placemark> with the given <Point>, DateTime and <styleUrl>.
// A <TimeStamp> is created from the DateTime and <ExtendedData> fields are
// created for date and time.
kmldom::PlacemarkPtr CreatePointPlacemarkWithTimeStamp(
    const kmldom::PointPtr& point, const kmlbase::DateTime& date_time,
    const char* style_id);

// Create a Region with LatLonAltBox set to the given bounds and Lod
// set to the given values.  This is a "2D" Region because no altitude
// mode is set which defaults the LatLonAltBox to clampToGround.
kmldom::RegionPtr CreateRegion2d(double north, double south,
                                 double east, double west,
                                 double minlodpixels, double maxlodpixels);


// Creates a <gx:FlyTo> element which has the specified <gx:duration> and the
// specified AbstractView.
kmldom::GxFlyToPtr CreateFlyTo(const kmldom::AbstractViewPtr& abstractview,
                               double duration);

// Creates a <gx:FlyTo> element which has the specified <gx:duration> and a
// an AbstractView. If the feature has a existing AbstractView it is used,
// otherwise a <LookAt> is computed from the spatial extents of the feature. The
// LookAt's altitude, heading and tilt are set to 0.0 and the altitudeMode is
// set to relativeToGroud. Returns NULL if the feature has no specified
// AbstractView and none can be computed.
// See kmlengine::ComputeFeatureLookAt for details of how the LookAt is
// generated.
kmldom::GxFlyToPtr CreateFlyToForFeature(const kmldom::FeaturePtr& feature,
                                         double duration);

// Creates a <gx:Wait> with a <gx:duration> of the specified value.
kmldom::GxWaitPtr CreateWait(double duration);

// This gets the value of the given name from the ExtendedData/Data as
// described above.  If there is no ExtendedData or no Data element with
// the given name false is returned.
bool GetExtendedDataValue(const kmldom::FeaturePtr& feature,
                          const string& name,
                          string* value);

// This sets the ExtendedData element of the feature to hold the given name
// value as a Data element as described above.  NOTE: Any previous ExtendedData
// is delete from this feature.
void SetExtendedDataValue(const string& name, const string& value,
                          kmldom::FeaturePtr feature);

// Returns a simplification of coordinates elements. merge_tolerance specifies
// a distance (in meters) within which adjacent coordinates tuples will be
// merged. If set to 0, no merge will occur.
// For example, assume we have coordinates of:
// (0,0,0 0,0,0 2,2,2 5,5,5 6,6,6 9,9,9)
// where the first two coordinates elements are coincident.
// If SimplifyCoordinates is called with a merge_tolerance of 1.0, the
// coincident points will be elided and the returned coordinates will be:
// (0,0,0 2,2,2 5,5,5 6,6,6 9,9,9)
// Since a 1 x 1 degree square near the equator has a diagonal of around
// 157,147m, if SimplifyCoordinates is called with a merge
// tolerance of 160000 the points at (5,5,5 6,6,6) will also be elided and the
// returned coordinates will be:
// (0,0,0 2,2,2 5,5,5 9,9,9)
void SimplifyCoordinates(const kmldom::CoordinatesPtr& src,
                         kmldom::CoordinatesPtr dest, double merge_tolerance);

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_CONVENIENCE_H__
