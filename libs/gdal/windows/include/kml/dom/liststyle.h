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

// This file contains the declaration of the ItemIcon and ListStyle elements.

#ifndef KML_DOM_LISTSTYLE_H__
#define KML_DOM_LISTSTYLE_H__

#include <vector>
#include "kml/base/color32.h"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/object.h"
#include "kml/dom/substyle.h"
#include "kml/base/util.h"

namespace kmldom {

class Visitor;
class VisitorDriver;

// <ItemIcon>
class ItemIcon : public Object {
 public:
  virtual ~ItemIcon();
  virtual KmlDomType Type() const { return Type_ItemIcon; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_ItemIcon || Object::IsA(type);
  }

  // <state>
  // Note that <state> within <ItemIcon> is an oddity within KML. It is the
  // only instance of an element whose character data is an array of
  // enumerations.
  //
  // Also note that since the element has a default enumeration
  // (<state>open</state>) the API usage is a little different. Calling
  // add_state(...) will simply append to the array of state enums. If you
  // initalize an ItemIcon object and wish to give it an explicit state, e.g.
  // <state>closed</state>, you should call clear_state() before using
  // add_state(...).
  //
  // State enumerations must be space-delimited. New lines, tabs, etc. are not
  // supported. This is consistent with the use of xsd:list in the KML schema.
  int get_state_array_at(size_t index) const {
    return state_array_[index];
  }
  size_t get_state_array_size() const {
    return state_array_.size();
  }
  bool has_state() const {
    return has_state_;
  }
  void add_state(int state) {
    state_array_.push_back(state);
    has_state_ = true;
  }
  // Note that clear_state will empty ALL stored state enums and thus does
  // not return the element to its default value of <state>open</state>.
  void clear_state() {
    state_array_.clear();
    has_state_ = false;
  }

  // <href>
  const string& get_href() const {
    return href_;
  }
  bool has_href() const {
    return has_href_;
  }
  void set_href(const string& href) {
    href_ = href;
    has_href_ = true;
  }
  void clear_href() {
    href_.clear();
    has_href_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  ItemIcon();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serialize) const;
  std::vector<int> state_array_;
  bool has_state_;
  string href_;
  bool has_href_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ItemIcon);
};

// <ListStyle>
class ListStyle : public SubStyle {
 public:
  virtual ~ListStyle();
  virtual KmlDomType Type() const { return Type_ListStyle; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_ListStyle || SubStyle::IsA(type);
  }

  // <listItemType>
  int get_listitemtype() const {
    return listitemtype_;
  }
  bool has_listitemtype() const {
    return has_listitemtype_;
  }
  void set_listitemtype(int listitemtype) {
    listitemtype_ = listitemtype;
    has_listitemtype_ = true;
  }
  void clear_listitemtype() {
    listitemtype_ = LISTITEMTYPE_CHECK;
    has_listitemtype_ = false;
  }

  // <bgColor>
  const kmlbase::Color32& get_bgcolor() const {
    return bgcolor_;
  }
  bool has_bgcolor() const {
    return has_bgcolor_;
  }
  void set_bgcolor(const kmlbase::Color32& bgcolor) {
    bgcolor_ = bgcolor;
    has_bgcolor_ = true;
  }
  void clear_bgcolor() {
    bgcolor_ = kmlbase::Color32(0xffffffff);
    has_bgcolor_ = false;
  }

  // <ItemIcon>
  void add_itemicon(const ItemIconPtr& itemicon) {
    AddComplexChild(itemicon, &itemicon_array_);
  }

  size_t get_itemicon_array_size() const {
    return itemicon_array_.size();
  }

  const ItemIconPtr& get_itemicon_array_at(size_t index) const {
    return itemicon_array_[index];
  }

  // <maxSnippetLines>
  int get_maxsnippetlines() const {
    return maxsnippetlines_;
  }
  bool has_maxsnippetlines() const {
    return has_maxsnippetlines_;
  }
  void set_maxsnippetlines(int maxsnippetlines) {
    maxsnippetlines_ = maxsnippetlines;
    has_maxsnippetlines_ = true;
  }
  void clear_maxsnippetlines() {
    maxsnippetlines_ = 2;
    has_maxsnippetlines_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  ListStyle();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serialize) const;
  int listitemtype_;
  bool has_listitemtype_;
  kmlbase::Color32 bgcolor_;
  bool has_bgcolor_;
  std::vector<ItemIconPtr> itemicon_array_;
  int maxsnippetlines_;
  bool has_maxsnippetlines_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ListStyle);
};

}  // end namespace kmldom

#endif // KML_DOM_LISTSTYLE_H__
