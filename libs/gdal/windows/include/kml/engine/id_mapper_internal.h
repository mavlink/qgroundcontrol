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

// This file contains the declaration of the internal IdMapper class.
// Do not use this class in application code.  See id_mapper.h for the public
// functions.

#ifndef KML_ENGINE_ID_MAPPER_INTERNAL_H__
#define KML_ENGINE_ID_MAPPER_INTERNAL_H__

#include "kml/base/string_util.h"
#include "kml/dom.h"
#include "kml/dom/serializer.h"
#include "kml/engine/engine_types.h"

namespace kmlengine {

// This class walks the element hierarchy and assigns any Object with an id
// to the given required ObjectIdMap.  Objects with duplicate ids are appended
// to the given ElementVector if one is supplied.
class IdMapper : public kmldom::Serializer {
 public:
  IdMapper(ObjectIdMap* object_id_map, ElementVector* dup_id_vector)
    : object_id_map_(object_id_map),
      dup_id_vector_(dup_id_vector) {
  }

  // This is the Serializer method used to recurse on each child element.
  virtual void SaveElement(const kmldom::ElementPtr& element);

 private:
  ObjectIdMap* object_id_map_;
  ElementVector* dup_id_vector_;
};

}  // end namespace kmlengine

#endif  // KML_ENGINE_ID_MAPPER_INTERNAL_H__
