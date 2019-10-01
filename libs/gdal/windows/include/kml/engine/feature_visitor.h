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

// This file contains the declarations of the GetRootFeature(),
// VisitFeatureHierarchy() functions and FeatureVisitor base class.

#ifndef KML_ENGINE_FEATURE_VISITOR_H__
#define KML_ENGINE_FEATURE_VISITOR_H__

#include "kml/dom.h"

namespace kmlengine {

// This returns the root Feature of the given KML hierarchy.  If root is
// neither of Type_Kml nor Type_Feature NULL is returned.
const kmldom::FeaturePtr GetRootFeature(const kmldom::ElementPtr& root);

// This is the base class for use with VisitFeatureHierarchy.  Derive your own
// class and implement VisitFeature and pass the instance of your class to
// VisitFeatureHierarchy
class FeatureVisitor {
 public:
  virtual ~FeatureVisitor() {}
  virtual void VisitFeature(const kmldom::FeaturePtr& feature) {}
};

// Visit the Feature hierarchy rooted at feature calling the VisitFeature()
// method of the given FeatureVisitor in depth-first order.
void VisitFeatureHierarchy(const kmldom::FeaturePtr& feature,
                           FeatureVisitor& feature_visitor);

}  // end namespace kmlengine

#endif  // KML_ENGINE_FEATURE_VISITOR_H__
