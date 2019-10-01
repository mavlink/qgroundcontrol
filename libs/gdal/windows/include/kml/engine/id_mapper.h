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

// This file contains the declaration of the public id mapping functions.

#ifndef KML_ENGINE_ID_MAPPER_H__
#define KML_ENGINE_ID_MAPPER_H__

#include "kml/base/string_util.h"
#include "kml/dom.h"
#include "kml/engine/engine_types.h"

namespace kmlengine {

// This function creates a map of all Objects with ids in the given element
// hierarchy.  If an element_vector is supplied it will be given all elements
// with duplicate ids.
void MapIds(const kmldom::ElementPtr& root, ObjectIdMap* object_id_map,
            ElementVector* element_vector);

// This function clears the id= attribute of every Object with an id in the
// given element hierarchy.  This function should be used with caution!
const kmldom::ElementPtr& ClearIds(const kmldom::ElementPtr& root);

// This function changes ALL id='s in the given ObjectIdMap.  If the
// Object's id= is not found in the given id_map that object's id= is cleared.
// If the Object's id= _is_ found in the id_map then that Object's id= is
// changed to the map'ed id.  If an output_object_id_map is supplied each
// remapped object is added to that map.  This returns the number of objects
// whose ids were cleared.  This function should be used with caution!
int RemapIds(const ObjectIdMap& input_object_id_map,
             const kmlbase::StringMap& id_map,
             ObjectIdMap* output_object_id_map);

}  // end namespace kmlengine

#endif  // KML_ENGINE_ID_MAPPER_H__
