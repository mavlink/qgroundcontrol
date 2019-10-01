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

// This file contains the declaration of the CreateBalloonText function.

// Every KML Feature may have an associated "description balloon". By default,
// this is an HTML composition of the Feature's name and description. It may
// be overridden with the use of BalloonStyle, and may further be modified
// by the use of entity replacements (such as "$[name]") and by supplying
// various combinations of ExtendedData elements.
// A useful guide for the various combinations of the latter can be found here:
// http://code.google.com/apis/kml/documentation/extendeddata.html
// For more information, see the OGC KML 2.2 specificication. In particular,
// sections 6.5 and 9.1.3.10.

#ifndef KML_ENGINE_FEATURE_BALLOON_H__
#define KML_ENGINE_FEATURE_BALLOON_H__

#include "kml/dom/feature.h"
#include "kml/engine/kml_file.h"

namespace kmlengine {

// Returns the markup for a Feature's "description balloon" (or "info window",
// in Google Maps-speak) as an HTML-formatted string. The text is composited
// from the KML roughly as follows:
// - The feature's styles are resolved and merged
// - All entity references are expanded to their full form. For more
// information on entity references, see:
// http://code.google.com/apis/kml/documentation/kmlreference.html#text
// - If a <BalloonStyle> and <description> are supplied, the description with
// expanded entities is returned. Otherwise, a default name and description
// balloon is created, identical to that shown by Google Earth 5.1 and earlier.
// - If no <description> is supplied but the feature has <ExtendedData>, a
// description is created from that ExtendedData similar to Google Earth's
// behavior. The format is:
// data_displayname | data_value (if displayName)
// data_name | data_value (if  not displayName)
// schema_name:simplefield_displayName | simpledata_value (if displayName)
// schema_name:simplefield_name | simpledata_value (if not displayName)
//
// It is the caller's responsibility to ensure that the feature exists within
// the kml_file.
string CreateBalloonText(const KmlFilePtr& kml_file,
                         const kmldom::FeaturePtr& feature);

}  // end namespace kmlengine

#endif  // KML_ENGINE_FEATURE_BALLOON_H__
