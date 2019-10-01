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

#ifndef KML_DOM_STYLE_H__
#define KML_DOM_STYLE_H__

#include "kml/dom/balloonstyle.h"
#include "kml/dom/iconstyle.h"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/labelstyle.h"
#include "kml/dom/liststyle.h"
#include "kml/dom/linestyle.h"
#include "kml/dom/polystyle.h"
#include "kml/dom/styleselector.h"

namespace kmldom {

class Serializer;
class Visitor;
class VisitorDriver;

class Style : public StyleSelector {
 public:
  virtual ~Style();
  virtual KmlDomType Type() const { return Type_Style; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Style || StyleSelector::IsA(type);
  }

  // <IconStyle>
  const IconStylePtr& get_iconstyle() const { return iconstyle_; }
  bool has_iconstyle() const { return iconstyle_ != NULL; }
  void set_iconstyle(const IconStylePtr& iconstyle) {
    SetComplexChild(iconstyle, &iconstyle_);
  }
  void clear_iconstyle() {
    set_iconstyle(NULL);
  }

  // <LabelStyle>
  const LabelStylePtr& get_labelstyle() const { return labelstyle_; }
  bool has_labelstyle() const { return labelstyle_ != NULL; }
  void set_labelstyle(const LabelStylePtr& labelstyle) {
    SetComplexChild(labelstyle, &labelstyle_);
  }
  void clear_labelstyle() {
    set_labelstyle(NULL);
  }

  // <LineStyle>
  const LineStylePtr& get_linestyle() const { return linestyle_; }
  bool has_linestyle() const { return linestyle_ != NULL; }
  void set_linestyle(const LineStylePtr& linestyle) {
    SetComplexChild(linestyle, &linestyle_);
  }
  void clear_linestyle() {
    set_linestyle(NULL);
  }

  // <PolyStyle>
  const PolyStylePtr& get_polystyle() const { return polystyle_; }
  bool has_polystyle() const { return polystyle_ != NULL; }
  void set_polystyle(const PolyStylePtr& polystyle) {
    SetComplexChild(polystyle, &polystyle_);
  }
  void clear_polystyle() {
    set_polystyle(NULL);
  }

  // <BalloonStyle>
  const BalloonStylePtr& get_balloonstyle() const { return balloonstyle_; }
  bool has_balloonstyle() const { return balloonstyle_ != NULL; }
  void set_balloonstyle(const BalloonStylePtr& balloonstyle) {
    SetComplexChild(balloonstyle, &balloonstyle_);
  }
  void clear_balloonstyle() {
    set_balloonstyle(NULL);
  }

  // <ListStyle>
  const ListStylePtr& get_liststyle() const { return liststyle_; }
  bool has_liststyle() const { return liststyle_ != NULL; }
  void set_liststyle(const ListStylePtr& liststyle) {
    SetComplexChild(liststyle, &liststyle_);
  }
  void clear_liststyle() {
    set_liststyle(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Style ();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  IconStylePtr iconstyle_;
  LabelStylePtr labelstyle_;
  LineStylePtr linestyle_;
  PolyStylePtr polystyle_;
  BalloonStylePtr balloonstyle_;
  ListStylePtr liststyle_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Style);
};

}  // end namespace kmldom

#endif  // KML_DOM_STYLE_H__
