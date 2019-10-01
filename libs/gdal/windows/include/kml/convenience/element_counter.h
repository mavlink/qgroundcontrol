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

#ifndef KML_CONVENIENCE_ELEMENT_COUNTER_H__
#define KML_CONVENIENCE_ELEMENT_COUNTER_H__

#include <map>
#include "kml/dom.h"

namespace kmlconvenience {

// This map is used to hold the occurrence count for each element.
typedef std::map<kmldom::KmlDomType, int> ElementCountMap;

// This ParserObserver uses the NewElement() method to count the number of
// ocurrences of each element.
class ElementCounter : public kmldom::ParserObserver {
 public:
  ElementCounter(ElementCountMap* element_count_map)
    : element_count_map_(*element_count_map) {
  }

  // ParserObserver::NewElement()
  virtual bool NewElement(const kmldom::ElementPtr& element) {
    if (element_count_map_.find(element->Type()) == element_count_map_.end()) {
      element_count_map_[element->Type()] = 1;
    } else {
      element_count_map_[element->Type()] += 1;
    }
    return true;  // Always return true to keep parsing.
  }

 private:
  ElementCountMap& element_count_map_;
};

}  // end namespace kmlconvenience

#endif // KML_CONVENIENCE_ELEMENT_COUNTER_H__
