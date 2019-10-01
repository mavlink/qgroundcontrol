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

// This file contains the declaration of the MergeElements() and MergeFields()
// functions.
// TODO: move these to an engine-specific kml_funcs.h

#ifndef KML_ENGINE_MERGE_H__
#define KML_ENGINE_MERGE_H__

#include "kml/dom.h"

namespace kmlengine {

// Each set simple child of source is set in target.  If a field is not set
// (has_xxx() == false) it is not changed in the target.   While the intended
// usage is for both source and target to be of the same complex type this does
// not need to be the case.  Thus, it is possible to set the latitude of Camera
// from a LookAt.
void MergeFields(const kmldom::ElementPtr& source, kmldom::ElementPtr target);

// This function implements a deep merge of all simple and complex child
// elements of source into the corresponding children of target.  The source
// is left unchanged upon return.  Any complex elements added to target are
// complete clones of those from the source.  No complex elements are ever
// shared between elements.  All array value elements in the source are
// appended to the corresponding array in the target.  Any substitution group
// elements (Placemark/Geometry) are _replaced_ in the target; for example, a
// Placemark/Point in the target will be fully replaced with a
// Placemark/LineString Clone()'ed from the source.  Each matching complex
// child element of source and target is recursively Merge()'ed.  Each field of
// source is merged into target (using MergeFields).  The source and target
// element type do not need to match.  Elements from source unknown to target
// are handled the same as a parse of unknown elements into the target and
// are similarily preserved for serialization.
void MergeElements(const kmldom::ElementPtr& source, kmldom::ElementPtr target);

}  // end namespace kmlengine

#endif  // KML_ENGINE_MERGE_H__
