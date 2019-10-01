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

// This file contains the declarations of the Pair and StyleMap elements.

#ifndef KML_DOM_STYLEMAP_H__
#define KML_DOM_STYLEMAP_H__

#include <vector>
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/object.h"
#include "kml/dom/styleselector.h"

namespace kmldom {

class Serializer;
class Visitor;
class VisitorDriver;

// <Pair>
class Pair : public Object {
 public:
  virtual ~Pair();
  virtual KmlDomType Type() const { return Type_Pair; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Pair || Object::IsA(type);
  }

  // <key>
  int get_key() const {
    return key_;
  }
  bool has_key() const {
    return has_key_;
  }
  void set_key(int key) {
    key_ = key;
    has_key_ = true;
  }
  void clear_key() {
    key_ = STYLESTATE_NORMAL;
    has_key_ = false;
  }

  // <styleUrl>
  const string& get_styleurl() const {
    return styleurl_;
  }
  bool has_styleurl() const {
    return has_styleurl_;
  }
  void set_styleurl(const string& styleurl) {
    styleurl_ = styleurl;
    has_styleurl_ = true;
  }
  void clear_styleurl() {
    styleurl_.clear();
    has_styleurl_ = false;
  }

  // StyleSelector
  const StyleSelectorPtr& get_styleselector() const { return styleselector_; }
  bool has_styleselector() const { return styleselector_ != NULL; }
  void set_styleselector(const StyleSelectorPtr& styleselector) {
    SetComplexChild(styleselector, &styleselector_);
  }
  void clear_styleselector() {
    set_styleselector(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Pair();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  int key_;
  bool has_key_;
  string styleurl_;
  bool has_styleurl_;
  StyleSelectorPtr styleselector_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Pair);
};

// <StyleMap>
class StyleMap : public StyleSelector {
 public:
  virtual ~StyleMap();
  virtual KmlDomType Type() const { return Type_StyleMap; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_StyleMap || StyleSelector::IsA(type);
  }

  void add_pair(const PairPtr& pair) {
    AddComplexChild(pair, &pair_array_);
  }

  size_t get_pair_array_size() const {
    return pair_array_.size();
  }

  const PairPtr& get_pair_array_at(size_t index) const {
    return pair_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  StyleMap();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  std::vector<PairPtr> pair_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(StyleMap);
};

}  // end namespace kmldom

#endif  // KML_DOM_STYLEMAP_H__
