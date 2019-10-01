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

// This file contains the declaration of the RegionHandler abstract base class.

#ifndef KML_REGIONATOR_REGION_HANDLER_H__
#define KML_REGIONATOR_REGION_HANDLER_H__

#include "kml/dom.h"

namespace kmlregionator {

// This abstract base class defines the interface used by the Regionator in
// calling out to the class implementing the "Regionation" of a dataset.
// The Regionator creates each KML file with Region-based NetworkLinks
// and queries the implementation for the KML Feature to show in each Region.
// A given specific regionator inherits from this class and implements the
// methods as indicated below.  Overall usage is:
//   // Create a specific RegionHandler:
//   class MyRegionHandler : public RegionHandler {...};
//   MyRegionHandler my_region_handler;
//   // The Regionator walks down the Region hierarchy starting at the
//   // specified Region calling the RegionHandler methods for each child
//   // Region in the hierarchy.  It is up to the HasData() method to
//   // eventually terminate the walk of the Region hierarchy.
//   kmldom::RegionPtr root_region;
//   Regionator regionator(&my_region_handler, root_region);
//   regionator.Regionate(NULL);  // Or supply a specific output directory.
//   // All "regionated" KML is now available wherever MyRegionHandler
//   // saved each file passed in to SaveKml().
class RegionHandler {
 public:
  virtual ~RegionHandler() {};  // Silence compiler warning.

  // This method is the first one called for a given Region.  The implementation
  // of this method should return true if there is data in this Region and/or
  // below this Region.  The implementation must eventually return false
  // for the Region walk to complete.  This method is called _before_ all
  // children are visited.
  virtual bool HasData(const kmldom::RegionPtr& region) = 0;

  // This method is always called if the HasData() method returned true.
  // The implementation of this method should return the Feature for this
  // Region.  This method is called _after_ all children are visited.
  virtual kmldom::FeaturePtr GetFeature(const kmldom::RegionPtr& region) = 0;

  // This method is called for each Region with the KML data for a given
  // Region along with the file name that the parent NetworkLink will use
  // to fetch the file.  It is implementation dependent just how the KML
  // is saved, but the exact name in the filename argument should be used
  // with no modification.
  // TODO: provide a flag to specify use of .kmz.
  virtual void SaveKml(const kmldom::KmlPtr& kml,
                       const string& filename) = 0;
};

}  // end namespace kmlregionator

#endif // KML_REGIONATOR_REGION_HANDLER_H__

