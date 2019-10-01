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

// This file contains the implementation of the Csv class for parsing CSV
// files and generating Point Placemark KML.
// NOTE: The CsvFile class is deprecated.  Use CsvParser in new code.

#include <vector>
#include "kml/base/util.h"

namespace kmlconvenience {

class FeatureList;

// This class converts a CSV file into a FeatureList.
// Usage:
//   FeatureList feature_list;
//   CsvFile csv_file(&feature_list);
//   csv_file.ParseCsvFile("input.csv");
// A FeatureList can be used with the FeatureListRegionHandler or directly
// with a KML Container.  See feature_list.h for more information.
// NOTE: This class is deprecated.  Use CsvParser in new code.
class CsvFile {
 public:
  CsvFile(kmlconvenience::FeatureList* feature_list)
      : feature_list_(feature_list) {}

  void ParseCsvLine(const string& csv_line);

  // Create a Point Placemark for each line of the given CSV file.
  void ParseCsvFile(const char* filename);

 private:
  kmlconvenience::FeatureList* feature_list_;
};

}  // end namespace kmlconvenience
