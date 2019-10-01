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

// This file contains the declaration of the ComputeFeatureLookAt function.

#ifndef KML_ENGINE_FEATURE_VIEW_H__
#define KML_ENGINE_FEATURE_VIEW_H__

#include "kml/dom/abstractview.h"
#include "kml/dom/feature.h"

namespace kmlengine {

class Bbox;

// Returns a <LookAt> element computed from the spatial extents of a feature.
// The LookAt's altitude, heading and tilt are set to 0.0, and the altitudeMode
// is set to relativeToGroud. The range is computed such that the feature will
// be within a viewport with a field of view of 60 deg and is clamped to a
// minimum of 1,000 meters. Returns NULL if the feature has no specified
// AbstractView and none can be computed.
kmldom::LookAtPtr ComputeFeatureLookAt(const kmldom::FeaturePtr& feature);

// Returns a <LookAt> element computed from the spatial extents of a Bbox.
// See ComputeFeatureLookAt for the details of how the LookAt is created.
kmldom::LookAtPtr ComputeBboxLookAt(const Bbox& bbox);

}  // end namespace kmlengine

#endif  // KML_ENGINE_FEATURE_VIEW_H__
