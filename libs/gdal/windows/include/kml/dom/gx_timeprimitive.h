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

// This file contains the declarations for the gx:TimeStamp and gx:TimeSpan
// elements.

#ifndef KML_DOM_GX_TIMEPRIMITIVE_H__
#define KML_DOM_GX_TIMEPRIMITIVE_H__

#include <string>
#include "kml/base/xml_namespaces.h"
#include "kml/dom/kml22.h"
#include "kml/dom/object.h"
#include "kml/dom/timeprimitive.h"

namespace kmldom {

class Serializer;
class Visitor;

// <gx:TimeSpan>
class GxTimeSpan : public TimeSpan {
 public:
  virtual ~GxTimeSpan();
  static KmlDomType ElementType() {
    return Type_GxTimeSpan;
  }
  virtual KmlDomType Type() const { return Type_GxTimeSpan; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_GxTimeSpan || TimeSpan::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  GxTimeSpan();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(GxTimeSpan);
};

// <gx:TimeStamp>
class GxTimeStamp : public TimeStamp {
 public:
  virtual ~GxTimeStamp();
  static KmlDomType ElementType() {
    return Type_GxTimeStamp;
  }
  virtual KmlDomType Type() const { return Type_GxTimeStamp; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_GxTimeStamp || TimeStamp::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  GxTimeStamp();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(GxTimeStamp);
};

}  // end namespace kmldom

#endif  // KML_DOM_GX_TIMEPRIMITIVE_H__
