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

#ifndef KML_DOM_SCHEMA_H__
#define KML_DOM_SCHEMA_H__

#include <vector>
#include "kml/dom/element.h"
#include "kml/dom/object.h"
#include "kml/dom/kml22.h"
#include "kml/base/util.h"

namespace kmlbase {
class Attributes;
}

namespace kmldom {

class Visitor;
class VisitorDriver;

// <SimpleField>
class SimpleField : public BasicElement<Type_SimpleField> {
 public:
  virtual ~SimpleField();

  const string& get_type() const { return type_; }
  bool has_type() const { return has_type_; }
  void set_type(const string& value) {
    type_ = value;
    has_type_ = true;
  }
  void clear_type() {
    type_.clear();
    has_type_ = false;
  }

  const string& get_name() const { return name_; }
  bool has_name() const { return has_name_; }
  void set_name(const string& value) {
    name_ = value;
    has_name_ = true;
  }
  void clear_name() {
    name_.clear();
    has_name_ = false;
  }

  const string& get_displayname() const { return displayname_; }
  bool has_displayname() const { return has_displayname_; }
  void set_displayname(const string& value) {
    displayname_ = value;
    has_displayname_ = true;
  }
  void clear_displayname() {
    displayname_.clear();
    has_displayname_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 protected:
  SimpleField();
  virtual void AddElement(const ElementPtr& element);
  virtual void ParseAttributes(kmlbase::Attributes* attributes);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  virtual void SerializeAttributes(kmlbase::Attributes* attributes) const;

 private:
  friend class KmlFactory;
  friend class KmlHandler;
  string type_;
  bool has_type_;
  string name_;
  bool has_name_;
  string displayname_;
  bool has_displayname_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(SimpleField);
};

// <gx:SimpleArrayField>
class GxSimpleArrayField : public SimpleField {
 public:
  virtual ~GxSimpleArrayField();
  virtual KmlDomType Type() const { return Type_GxSimpleArrayField; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_GxSimpleArrayField || SimpleField::IsA(type);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  GxSimpleArrayField();
  friend class KmlHandler;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(GxSimpleArrayField);
};

// <Schema>
// Note: in the XSD Schema is not an Object. We inherit from Object here
// so it appears in the parsed object map and is easily accessible.
class Schema : public Object {
 public:
  virtual ~Schema();
  virtual KmlDomType Type() const { return Type_Schema; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Schema || Object::IsA(type);
  }

  const string& get_name() const { return name_; }
  bool has_name() const { return has_name_; }
  void set_name(const string& value) {
    name_ = value;
    has_name_ = true;
  }
  void clear_name() {
    name_.clear();
    has_name_ = false;
  }

  void add_simplefield(const SimpleFieldPtr& simplefield) {
    AddComplexChild(simplefield, &simplefield_array_);
  }

  size_t get_simplefield_array_size() const {
    return simplefield_array_.size();
  }

  const SimpleFieldPtr& get_simplefield_array_at(size_t index) const {
    return simplefield_array_[index];
  }

  void add_gx_simplearrayfield(
      const GxSimpleArrayFieldPtr& gx_simplearrayfield) {
    AddComplexChild(gx_simplearrayfield, &gx_simplearrayfield_array_);
  }

  size_t get_gx_simplearrayfield_array_size() const {
    return gx_simplearrayfield_array_.size();
  }

  const GxSimpleArrayFieldPtr& get_gx_simplearrayfield_array_at(
      size_t index) const {
    return gx_simplearrayfield_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Schema();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  virtual void ParseAttributes(kmlbase::Attributes* attributes);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  virtual void SerializeAttributes(kmlbase::Attributes* attributes) const;
  string name_;
  bool has_name_;
  std::vector<SimpleFieldPtr> simplefield_array_;
  std::vector<GxSimpleArrayFieldPtr> gx_simplearrayfield_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Schema);
};

}  // namespace kmldom

#endif  // KML_DOM_SCHEMA_H__
