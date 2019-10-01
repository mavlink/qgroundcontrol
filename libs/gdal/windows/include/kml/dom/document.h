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

#ifndef KML_DOM_DOCUMENT_H__
#define KML_DOM_DOCUMENT_H__

#include <vector>
#include "kml/dom/container.h"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/schema.h"
#include "kml/dom/styleselector.h"

namespace kmldom {

class Visitor;
class VisitorDriver;

class Document : public Container {
 public:
  virtual ~Document();
  virtual KmlDomType Type() const { return Type_Document; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Document || Container::IsA(type);
  }

  // <Schema>
  void add_schema(const SchemaPtr& schema) {
    AddComplexChild(schema, &schema_array_);
  }

  size_t get_schema_array_size() const {
    return schema_array_.size();
  }

  const SchemaPtr& get_schema_array_at(size_t index) const {
    return schema_array_[index];
  }

  SchemaPtr DeleteSchemaAt(size_t index) {
    return Element::DeleteFromArrayAt(&schema_array_, index);
  }

  // <Style> and <StyleMap>
  void add_styleselector(const StyleSelectorPtr& styleselector) {
    AddComplexChild(styleselector, &styleselector_array_);
  }

  size_t get_styleselector_array_size() const {
    return styleselector_array_.size();
  }

  const StyleSelectorPtr& get_styleselector_array_at(size_t index) const {
    return styleselector_array_[index];
  }

  StyleSelectorPtr DeleteStyleSelectorAt(size_t index) {
    return Element::DeleteFromArrayAt(&styleselector_array_, index);
  }

  // Note: If Document contains a StyleSelector, it is appended to Document's
  // array of StyleSelectors and is NOT handed up to Feature. The current
  // KML Spec/XSD is incorrect in that it gives any Feature this array
  // behaviour. Any Feature other than Document may have only ONE StyleSelector.

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Document();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  std::vector<SchemaPtr> schema_array_;
  std::vector<StyleSelectorPtr> styleselector_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Document);
};

}  // end namespace kmldom

#endif  // KML_DOM_DOCUMENT_H__
