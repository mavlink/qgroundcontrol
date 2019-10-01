// Copyright 2010, Google Inc. All rights reserved.
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

#ifndef KML_REGIONATOR_FEATURE_LIST_REGIONATOR_H__
#define KML_REGIONATOR_FEATURE_LIST_REGIONATOR_H__

#include <map>
#include "kml/dom.h"
#include "kml/convenience/feature_list.h"
#include "kml/engine.h"
#include "kml/regionator/regionator.h"
#include "kml/regionator/region_handler.h"

namespace kmlregionator {

// This class provides a "NULL" ProgressMonitor usable with
// FeatureListRegionator.
class NullProgress {
 public:
  // This method is called periodically from the FeatureListRegionator
  // providing the count of features regionated and the total.  The return
  // status indicates if regionation should proceed or immediately stop.
  bool RegionatorProgress(unsigned int count, unsigned int total) {
    // This RegionatorProgress always lets the regionation run to completion.
    return true;
  }
};

// This RegionHandler creates a Region-based NetworkLink hierarchy from a
// FeatureList.  YourProgressMonitor is any class which provides a
// RegionatorProgress method such as a class implementing a progress bar widget.
// Templates are used here to avoid requiring such a class inherit from any
// class within libkml.
//   YourProgressMonitor progress;  // Supplies a RegionatorProgress method.
//   FeatureListRegionator<YourProgressMonitor>::Regionate(feature_list,
//                                                         features_per_node,
//                                                         &progress,
//                                                         output_dir);
template<class ProgressMonitor = NullProgress>
class FeatureListRegionator : public RegionHandler {
 public:
  // RegionHandler::HasData()
  // This is called by the Regionator at the start of the given region.
  // Here we split out the first kMaxPer Features within this region into
  // a new FeatureList saved to a map based on the Region's id.
  virtual bool HasData(const kmldom::RegionPtr& region) {
    kmlconvenience::FeatureList this_region;
    if (feature_list_.RegionSplit(region, max_per_, &this_region) > 0) {
      kmldom::FolderPtr folder =
          kmldom::KmlFactory::GetFactory()->CreateFolder();
      this_region.Save(folder);
      feature_map_[region->get_id()] = folder;
      if (progress_monitor_) {
        return progress_monitor_->RegionatorProgress(
            feature_list_size_ - feature_list_.Size(), feature_list_size_);
      }
      return true;
    }
    return false;
  }

  // RegionHandler::GetFeature()
  // This is called by the Regionator at the end of the given region iff
  // HasData returned true for this region.
  virtual kmldom::FeaturePtr GetFeature(const kmldom::RegionPtr& region) {
    return feature_map_[region->get_id()];
  }

  // RegionHandler::SaveKml()
  // This is called by the Regionator to save the completed KML file.  We
  // simply write the file out into the file system into the current directory.
  virtual void SaveKml(const kmldom::KmlPtr& kml, const string& filename) {
    string kml_data(kmldom::SerializePretty(kml));
    kmlbase::File::WriteStringToFile(kml_data, filename);
  }

  // This static method permits a one line call to the regionator for the
  // data in the given FeatureList.  While a feature_list must always be
  // provided either or both of the ProgressMonitor or output_dir may be NULL.
  // The return status is that of the overall regionation.  See
  // Regionator::RegionateAligned for more information.
  static bool Regionate(kmlconvenience::FeatureList* feature_list,
                        unsigned int max_per, ProgressMonitor* progress_monitor,
                        const char* output_dir) {
    if (!feature_list) {
      return false;
    }
    // Create a root Region based on the bounding box of the FeatureList.
    kmlengine::Bbox bbox;
    feature_list->ComputeBoundingBox(&bbox);

    // The minLodPixels value of 256 means that a given node in the hierarchy
    // becomes visible at 256^2 pixels, hence max_per items in 256^2 pixels
    // of screen space.  The maxLodPixels value of -1 essentially requests
    // that the feature remain visible no matter how close the viewpoint.  The
    // overall effect is for features to accumulate as the viewpoint nears (and
    // also to "thin out" as the viewpoint retreats.  (Note that "nears" and
    // "retreats" are simplistic given that visibility is related also to tilt
    // and terrain).
    // For a deeper discussion of these matters please see:
    // http://code.google.com/apis/kml/documentation/regions.html
    kmldom::RegionPtr root = kmlconvenience::CreateRegion2d(bbox.get_north(),
                                                            bbox.get_south(),
                                                            bbox.get_east(),
                                                            bbox.get_west(),
                                                            256, -1);
    feature_list->Sort();
    FeatureListRegionator flr(feature_list, max_per, progress_monitor);
    return Regionator::RegionateAligned(flr, root, output_dir);
  }

 private:
  // Use the static Regionate method.
  FeatureListRegionator(kmlconvenience::FeatureList* feature_list,
                        unsigned int max_per,
                        ProgressMonitor* progress_monitor)
   : feature_list_(*feature_list),
     feature_list_size_(feature_list->Size()),
     max_per_(max_per),
     progress_monitor_(progress_monitor) {
  }

  kmlconvenience::FeatureList feature_list_;
  const size_t feature_list_size_;
  std::map<string, kmldom::FolderPtr> feature_map_;
  const unsigned int max_per_;
  ProgressMonitor* progress_monitor_;
};

}  // end namespace kmlregionator

#endif  // KML_REGIONATOR_FEATURE_LIST_REGIONATOR_H__
