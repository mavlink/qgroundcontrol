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

// This file contains the definition of the Qid class used to manage the
// Region hierarchy walked in the Regionator class.  This is an internal class.

#ifndef KML_REGIONATOR_REGIONATOR_QID_H__
#define KML_REGIONATOR_REGIONATOR_QID_H__

#include <sstream>
#include "kml/base/util.h"

namespace kmlregionator {

enum quadrant_t {
  NW,
  NE,
  SW,
  SE
};

const char* const kRootName = "q0";

// A Qid is simply a number to identify a Region.  There are methods on a Qid
// to create Qid's for the four children of a Region.
class Qid {
public:
  Qid() {}
  Qid(const string& qid) : qid_(qid) {}
  static Qid CreateRoot() {
    return Qid(kRootName);
  }
  Qid CreateChild(quadrant_t quadrant) const {
    std::stringstream ss;
    ss << quadrant;
    return Qid(qid_ + ss.str());
  }
  size_t depth() const {
    return qid_.size() - 1;
  }
  const string& str() const {
    return qid_;
  }
  bool IsRoot() {
    return qid_ == kRootName;
  }
private:
  string qid_;
};

}  // end namespace kmlregionator

#endif  // KML_REGIONATOR_REGIONATOR_QID_H__
