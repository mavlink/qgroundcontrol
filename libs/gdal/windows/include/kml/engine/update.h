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

#ifndef KML_ENGINE_UPDATE_H__
#define KML_ENGINE_UPDATE_H__

#include "kml/dom.h"
#include "kml/engine/kml_file.h"

namespace kmlengine {

// This provides in-place (destructive) processing of the given update against
// the given KmlFile.  In the case of NetworkLinkControl it is presumed the
// caller has checked Update's targetHref against KmlFile's url.
void ProcessUpdate(const kmldom::UpdatePtr& update, KmlFilePtr kml_file);

// This is the same as ProcessUpdate() except the caller provided StringMap is
// used to map the targetId='s in the Update before they are applied to the
// id='s in the KmlFile.  If a StringMap is supplied and a given targetId= does
// not have a mapping to an id= or if an Object with this id= does not exist in
// the KmlFile that particular Update target is quietly ignored.  While the
// target'ed Object's contents are Update'ed the id is not whether the
// targetId= is mapped or not.  If a NULL pointer is passed for the StringMap
// then no mapping are performed and this operates like ProcessUpdate().
void ProcessUpdateWithIdMap(const kmldom::UpdatePtr& update,
                            const kmlbase::StringMap* id_map,
                            KmlFilePtr kml_file);

// Clone each Feature in the source_container and append to the target.
void CopyFeatures(const kmldom::ContainerPtr& source_container,
                  kmldom::ContainerPtr target_container);

}  // namespace kmlengine

#endif  // KML_ENGINE_UPDATE_H__
