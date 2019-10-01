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

// This file contains the declarations of utility functions used in creating
// Region KML.
// TODO: the bulk of these are general purpose and should find their way to
// some more central place.

#ifndef KML_REGIONATOR_REGIONATOR_UTIL_H__
#define KML_REGIONATOR_REGIONATOR_UTIL_H__

#include "kml/base/util.h"
#include "kml/dom.h"
#include "kml/regionator/regionator_qid.h"

namespace kmlengine {
class Bbox;
}

namespace kmlregionator {

// Creates a copy of the given LatLonAltBox.
kmldom::LatLonAltBoxPtr CloneLatLonAltBox(const kmldom::LatLonAltBoxPtr& orig);

// Creates a copy of the given Lod.
kmldom::LodPtr CloneLod(const kmldom::LodPtr& orig);

// Creates a deep copy of the given Region.
kmldom::RegionPtr CloneRegion(const kmldom::RegionPtr& orig);

// This sets the bounds of the output aligned_llb to the lowest level node
// in a quadtree rooted at n=180, s=-180, e=180, w=-180.
bool CreateAlignedAbstractLatLonBox(const kmldom::AbstractLatLonBoxPtr& llb,
                                    kmldom::AbstractLatLonBoxPtr aligned_llb);

// Creates a Region whose LatLonAltBox is the specified quadrant of
// that in the parent.  The created Region's Lod is cloned from the parent.
kmldom::RegionPtr CreateChildRegion(const kmldom::RegionPtr& parent,
                                    quadrant_t quadrant);

// Create a Placemark with LineString based on the LatLonAltBox in the Region.
kmldom::PlacemarkPtr CreateLineStringBox(const string& name,
                                         const kmldom::RegionPtr& region);

// Create a NetworkLink to the given URL with a Region cloned from the
// given Region.
kmldom::NetworkLinkPtr CreateRegionNetworkLink(const kmldom::RegionPtr& region,
                                               const string& href);

// Create a Document with a Region cloned from the given Region.
kmldom::DocumentPtr CreateRegionDocument(const kmldom::RegionPtr& region);

}  // end namespace kmlregionator

#endif  // KML_REGIONATOR_REGIONATOR_UTIL_H__
