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

// WARNING: THE VISITOR API IMPLEMENTED IN THIS CLASS IS EXPERIMENTAL AND
// SUBJECT TO CHANGE WITHOUT WARNING.

#ifndef KML_DOM_VISITOR_DRIVER_H__
#define KML_DOM_VISITOR_DRIVER_H__

#include "kml/base/util.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/visitor.h"

namespace kmldom {

// A visitor driver controls the flow of a visitation over the dom element
// hierarchy. There is typically expected to be only a single driver for each
// visitation and currently there is not concept of chaining drivers (though
// visitors themselves could be chained within a single 'multi driver').
class VisitorDriver {
 protected:
  VisitorDriver();
  virtual ~VisitorDriver();

 public:
  // Handles the visitation of the subtree of elements rooted at the given
  // element. This method is invoked either by the user to initiate a visitation
  // or in response to a call to AcceptChildren() for an element.
  //
  // Typically when a driver visits an element it will, in some order:
  // - call accept() on the given element for some set of visitors
  // - call acceptChildren() on the given element, passing itself
  // However there is no requirement that either of these actually occur and
  // the driver is free to implement whatever semantics it chooses.
  virtual void Visit(const ElementPtr& element) = 0;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(VisitorDriver);
};

// A simple driver implementation that invokes a single visitor in pre-order
// traversal of an element hierarchy. The visitor will visit each element in a
// hierarchy before that element's children are visited.
class SimplePreorderDriver : public VisitorDriver {
 public:
  explicit SimplePreorderDriver(Visitor* visitor);
  virtual ~SimplePreorderDriver();

  virtual void Visit(const ElementPtr& element);

 private:
  Visitor* visitor_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(SimplePreorderDriver);
};


// A simple driver implementation that invokes a single visitor in post-order
// traversal of an element hierarchy. The visitor will visit each element in a
// hierarchy after that element's children have been visited.
class SimplePostorderDriver : public VisitorDriver {
 public:
  explicit SimplePostorderDriver(Visitor* visitor);
  virtual ~SimplePostorderDriver();

  virtual void Visit(const ElementPtr& element);

 private:
  Visitor* visitor_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(SimplePostorderDriver);
};

}  // namespace kmldom

#endif  // KML_DOM_VISITOR_DRIVER_H__
