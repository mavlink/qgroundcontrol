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

// This file contains the declaration of the FeatureList class.

#ifndef KML_CONVENIENCE_FEATURE_LIST_H__
#define KML_CONVENIENCE_FEATURE_LIST_H__

#include <list>
#include "kml/dom.h"
#include "kml/engine.h"

namespace kmlconvenience {

// This returns the value of the "Score" Data element as described above.
// This uses GetExtendedDataValue().
int GetFeatureScore(kmldom::FeaturePtr feature);

// This sets the value of the "Score" data element as described above.
// This uses SetExtendedDataValue().
void SetFeatureScore(const string& score, kmldom::FeaturePtr feature);

// STL list has constant time erase.
typedef std::list<kmldom::FeaturePtr> feature_list_t;

// This class provides an efficient data structure to gather, sort and
// split Features by bounding box.  Basic usage:
//   FeatureList feature_list;
//   for each feature:
//     SetFeatureScore(the_features_score, feature);
//     feature_list.PushBack(feature);
//   feature_list.Sort()
//   FeatureList features_in_some_bbox;
//   Bbox some_bbox;
//   feature_list.BboxSplit(bbox, how_many, &features_in_some_bbox);
//   FolderPtr folder;
//   features_in_some_bbox.Save(&folder);
//   folder->set_region(something-that-creates-a-Region-from-a-Bbox(some_bbox);
class FeatureList {
 public:
  // Append the feature to the end of the list.
  void PushBack(const kmldom::FeaturePtr& feature);

  // Split up to max features which are within the bounding box out to the
  // given FeatureList.  If a NULL FeatureList is supplied the features are
  // deleted from this FeatureList.  NOTE: This is DESTRUCTIVE with respect
  // to this FeatureList.
  size_t BboxSplit(const kmlengine::Bbox& bbox, size_t max,
                   FeatureList* output);

  // This calls BboxSplit based on a Bbox constructed from the LatLonAltBox
  // of the Region if it has one.  NOTE: This is DESTRUCTIVE.
  size_t RegionSplit(const kmldom::RegionPtr& region, size_t max,
                     FeatureList* output);

  // This sorts the features within this FeatureList based on score as set
  // in ExtendedData.
  void Sort();

  // This returns the number of features within the FeatureList.
  size_t Size() const;

  // This expands the bounds of the given Bbox to enclose all features in
  // this FeatureList.
  void ComputeBoundingBox(kmlengine::Bbox* bbox) const;

  // This appends all features to the given container.  Order is preserved.
  size_t Save(kmldom::ContainerPtr container) const;

 private:
  feature_list_t feature_list_;
};

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_FEATURE_LIST_H__
