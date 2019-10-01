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

#ifndef KML_DOM_LABELSTYLE_H__
#define KML_DOM_LABELSTYLE_H__

#include "kml/dom/colorstyle.h"
#include "kml/dom/kml22.h"
#include "kml/base/util.h"

namespace kmldom {

class Visitor;

class LabelStyle : public ColorStyle {
 public:
  virtual ~LabelStyle();
  virtual KmlDomType Type() const { return Type_LabelStyle; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_LabelStyle || ColorStyle::IsA(type);
  }

  // <scale>
  double get_scale() const {
    return scale_;
  }
  bool has_scale() const {
    return has_scale_;
  }
  void set_scale(double scale) {
    scale_ = scale;
    has_scale_ = true;
  }
  void clear_scale() {
    scale_ = 1.0;
    has_scale_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  LabelStyle();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serialize) const;
  double scale_;
  bool has_scale_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(LabelStyle);
};

}  // end namespace kmldom

#endif // KML_DOM_LABELSTYLE_H__
