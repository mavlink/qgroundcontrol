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

// This file contains the declaration of the internal UpdateProcecssor class.
// This is not intended for use in application code.  See update.h for info
// about kmlengine::ProcessUpdate().

#ifndef KML_ENGINE_UPDATE_PROCESSOR_H__
#define KML_ENGINE_UPDATE_PROCESSOR_H__

#include "kml/base/string_util.h"
#include "kml/dom/kml_ptr.h"

namespace kmlengine {

class KmlFile;

class UpdateProcessor {
 public:
  // Create an UpdateProcessor for a given KmlFile.  If an id_map is supplied
  // then all targetId='s in all Update operations are looked up there to find
  // the id=' used in the KmlFile.  The id='s found inside the KmlFile are never
  // changed by this class.
  UpdateProcessor(const KmlFile& kml_file, const kmlbase::StringMap* id_map)
    : kml_file_(kml_file),
      id_map_(id_map) {
  }

  // Process the given <Update> against the KmlFile associated with this
  // UpdateProcessor.  The <targetHref> is NOT examined.
  void ProcessUpdate(const kmldom::UpdatePtr& update);

  // Process the given <Change> against the KmlFile associated with this
  // UpdateProcessor.
  void ProcessUpdateChange(const kmldom::ChangePtr& change);

  // Process the given <Create> against the KmlFile associated with this
  // UpdateProcessor.
  void ProcessUpdateCreate(const kmldom::CreatePtr& create);

  // Process the given <Delete> against the KmlFile associated with this
  // UpdateProcessor.
  void ProcessUpdateDelete(const kmldom::DeletePtr& deleet);

  // This is a key reason for this class: to remap the targetId against
  // the supplied id map.  If the id_map this class was constructed with was
  // NULL then this simply returns the targetid.
  bool GetTargetId(const kmldom::ObjectPtr& object,
                   string* targetid) const;

 private:
  kmldom::FeaturePtr DeleteFeatureById(const string& id);
  const kmlengine::KmlFile& kml_file_;
  const kmlbase::StringMap* id_map_;
};

}  // end namespace kmlengine

#endif  //  KML_ENGINE_UPDATE_PROCESSOR_H__
