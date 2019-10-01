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

#ifndef KML_REGIONATOR_FEATURE_LIST_REGION_HANDLER_H__
#define KML_REGIONATOR_FEATURE_LIST_REGION_HANDLER_H__

#include <map>
#include "kml/dom.h"
#include "kml/convenience/feature_list.h"
#include "kml/engine.h"
#include "kml/regionator/region_handler.h"

namespace kmlregionator {

// This RegionHandler creates a Region-based NetworkLink hierarchy from a
// FeatureList.  See the FeatureList class comments for usage of this with
// a CSV file of point data.
class FeatureListRegionHandler : public RegionHandler {
 public:
  FeatureListRegionHandler(kmlconvenience::FeatureList* feature_list)
      : feature_list_(*feature_list) {}

  // TODO rename to RegionHandler::BeginRegion()
  // RegionHandler::HasData()
  // This is called by the Regionator at the start of the given region.
  // Here we split out the first kMaxPer Features within this region into
  // a new FeatureList saved to a map based on the Region's id.
  virtual bool HasData(const kmldom::RegionPtr& region);

  // TODO rename to RegionHandler::EndRegion()
  // RegionHandler::GetFeature()
  // This is called by the Regionator at the end of the given region iff
  // HasData returned true for this region.
  virtual kmldom::FeaturePtr GetFeature(const kmldom::RegionPtr& region);

  // RegionHandler::SaveKml()
  // This is called by the Regionator to save the completed KML file.  We
  // simply write the file out into the file system into the current directory.
  virtual void SaveKml(const kmldom::KmlPtr& kml, const string& filename);

 private:
  kmlconvenience::FeatureList feature_list_;
  std::map<string, kmldom::FolderPtr> feature_map_;
};

}  // end namespace kmlregionator

#endif  // KML_REGIONATOR_FEATURE_LIST_REGION_HANDLER_H__
