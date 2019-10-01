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

// This file contains the declaration of the Regionator class.  See the
// comments in region_handler.h for details on programming and regionator.cc
// for details of the operation.

#ifndef KML_REGIONATOR_REGIONATOR_H__
#define KML_REGIONATOR_REGIONATOR_H__

#include <map>
#include <vector>
#include "kml/dom.h"
#include "kml/regionator/region_handler.h"
#include "kml/regionator/regionator_qid.h"

namespace kmlregionator {

typedef std::vector<kmldom::RegionPtr> region_vector_t;

// The Regionator class is the API to the "regionator" algorithm.
class Regionator {
public:
  // A Regionator instance is created with a class derived from
  // RegionHandler and a root Region.
  Regionator(RegionHandler& rhandler, const kmldom::RegionPtr& region);
  ~Regionator();
  // This method starts the "regionation".  See region_handler.h for
  // details on how this class calls out to the RegionHandler.
  // The default output directory (output_directory == NULL) is the
  // current working directory of the caller. Returns true when regionation
  // has completed.  Each generated KML file has a rel="up" link to the root
  // KML file.  The root KML file has a rel="self" link.  See SetRootFilename
  // for the name of the root KML file.  Since data presented via Region-based
  // NetworkLinks used in this manner compounds it is of interest to "reach up"
  // all the way to the root upon discovering any descendant node.
  bool Regionate(const char* output_directory);

  // This method "regionates" using the given RegionHandler and region.  The
  // region is first aligned to the lowest level region in a quadtree rooted
  // at n=180, s=-180, e=180, w=-180.  All output files are saved to the
  // given directory if a non-NULL pointer is supplied.  Regionation progresses
  // the same whether nor not an output directory is supplied.  This also
  // adds a <LookAt> to the root node for the bounds of the data set as
  // described by the region used here.
  static bool RegionateAligned(RegionHandler& rhandler,
                               const kmldom::RegionPtr& region,
                               const char* output_directory);

  // By default, the resulting root filename will be "1.kml".  Provide an
  // override for that name with this method.  This file is also added as the
  // <atom:link> of every descendent kml.
  void SetRootFilename(const char *filename) { root_filename_ = filename; }

  // This <Region>'s <LatLonAltBox> is used as the basis for the <LookAt>
  // added to the root node of the generated hierarchy.  Without this there
  // is no explicit <LookAt>.
  void SetNaturalRegion(const kmldom::RegionPtr& region) {
    natural_region_ = region;
  }

private:
  kmldom::RegionPtr root_region_;
  // This calls _Regionate() for the given child of the parent Region.
  // This saves the child Region to the children vector if the child Region
  // has data.
  void Recurse(const kmldom::RegionPtr& parent, quadrant_t quadrant,
               region_vector_t* children);
  // This calls the RegionHandler for the given region.  If the RegionHandler
  // returns false from HasData() or NULL from GetFeature() this returns false
  // signalling that this Region has no data.
  bool _Regionate(const kmldom::RegionPtr& region);
  RegionHandler& rhandler_;
  // This returns the relative filename for the given Region.  A parent KML
  // file NetworkLink will look for a child with this name.
  string RegionFilename(const kmldom::RegionPtr& region);
  int region_count_;
  std::map<string,int> qid_map_;
  char* output_directory_;
  const char* root_filename_;
  kmldom::RegionPtr natural_region_;
};

}  // end namespace kmlregionator

#endif  // KML_REGIONATOR_REGIONATOR_H__
