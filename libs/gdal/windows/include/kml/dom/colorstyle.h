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

// This file contains the declaration of the ColorStyle element.

#ifndef KML_DOM_COLORSTYLE_H__
#define KML_DOM_COLORSTYLE_H__

#include "kml/dom/substyle.h"
#include "kml/base/color32.h"
#include "kml/dom/kml22.h"
#include "kml/base/util.h"

namespace kmldom {

// OGC KML 2.2 Standard: 12.7 kml:AbstractColorStyleGroup
// OGC KML 2.2 XSD: <element name="AbstractColorStyleGroup"...
class ColorStyle : public SubStyle {
 public:
  virtual ~ColorStyle();
  virtual KmlDomType Type() const { return Type_ColorStyle; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_ColorStyle || SubStyle::IsA(type);
  }

  // <color>
  const kmlbase::Color32& get_color() const {
    return color_;
  }
  bool has_color() const {
    return has_color_;
  }
  void set_color(const kmlbase::Color32& color) {
    color_ = color;
    has_color_ = true;
  }
  void clear_color() {
    color_ = kmlbase::Color32(0xffffffff);
    has_color_ = false;
  }

  // <colorMode>
  int get_colormode() const {
    return colormode_;
  }
  bool has_colormode() const {
    return has_colormode_;
  }
  void set_colormode(int colormode) {
    colormode_ = colormode;
    has_colormode_ = true;
  }
  void clear_colormode() {
    colormode_ = COLORMODE_NORMAL;
    has_colormode_ = false;
  }

 protected:
  // ColorStyle is abstract.
  ColorStyle();
  virtual void AddElement(const ElementPtr& element);
  virtual void Serialize(Serializer& serializer) const;

 private:
  kmlbase::Color32 color_;
  bool has_color_;
  int colormode_;
  bool has_colormode_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ColorStyle);
};

}  // end namespace kmldom

#endif // KML_COLORSTYLE_H_
