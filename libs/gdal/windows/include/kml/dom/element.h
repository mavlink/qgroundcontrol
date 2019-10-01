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

// This file contains the declaration of the Element and Field classes.
// The Element class is the base class for the KML Object Model.  All
// KML complex elements are derived from this class.  As the Element class
// members wnd methods indicate an Element always has a given type id, and
// a single parent Element.  Element itself holds all unknown XML for a given
// KML element including completely unknown XML, misplaced KML,
// and unknown attributes.  During parse a simple element is held for
// a short time in the Field specialization of Element.

#ifndef KML_DOM_ELEMENT_H__
#define KML_DOM_ELEMENT_H__

#include <vector>
#include "boost/scoped_ptr.hpp"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/visitor_driver.h"
#include "kml/base/util.h"
#include "kml/base/xml_element.h"

namespace kmlbase {
class Attributes;
}

namespace kmldom {

class Serializer;
class Visitor;
class Xsd;

// This is a KML-specific implementation of the somewhat abstracted
// kmlbase::XmlElement.
class Element : public kmlbase::XmlElement {
 public:
  virtual ~Element();
  virtual KmlDomType Type() const { return type_id_; }
  virtual bool IsA(KmlDomType type) const {
    return type == type_id_;
  }

  // This returns the element of which this is a child (if any).
  ElementPtr GetParent() const;

  // This is the concatenation of all character data found parsing this element.
  const string& get_char_data() const {
    return char_data_;
  }
  void set_char_data(const string& char_data) {
    char_data_ = char_data;
  }

  // TODO: AddElement() and ParseAttributes() should really be protected.

  // A derived class implements this to use with parsing.  Element is
  // either a complex or simple element which the given concrete element
  // can accept.  If the given element is a valid child the concrete element
  // takes ownership.  The given element is attached to the concrete parent
  // if it is a valid complex child.  If the element is a simple element
  // the character data is converted to the appropropriate simple type
  // and the passed element is discarded.  If the passed element is not
  // a valid child of the given concrete element the AddElement method
  // there should pass this up to its parent for consideration.  A misplaced
  // element will ultimately be attached to Element itself.
  virtual void AddElement(const ElementPtr& element);

  // A derived class implements this to use with parsing.  A given concrete
  // element examines the passed attributes for any it is aware of and
  // passes the attributes to its parent class and ultimately to Element
  // itself to preserve unknown attributes.  The caller takes ownership of
  // the passed attributes class instance and is expected to erase any items
  // it parses.
  virtual void ParseAttributes(kmlbase::Attributes* attributes);

  // A derived class implements this to use with serialization.  See
  // class Serializer for more information.
  virtual void Serialize(Serializer& serialize) const {}

  // A derived class uses this to use with serialization.  The derived
  // class adds its attributes to the given set and passes attributes
  // along to the parent and utlimately to Element itself to preserve
  // unknown attributes.
  virtual void SerializeAttributes(kmlbase::Attributes* attributes) const;

  // Each fully unknown element (and its children) is saved in raw XML form.
  void AddUnknownElement(const string& s);

  // Called by concrete elements to serialize unknown and/or misplaced
  // elements discovered at parse time.
  void SerializeUnknown(Serializer& serializer) const;

  // Returns the unknown elements.
  size_t get_unknown_elements_array_size() const {
    return unknown_elements_array_.size();
  }
  const string& get_unknown_elements_array_at(size_t i) const {
    return unknown_elements_array_[i];
  }

  // Returns the unknown legal (misplaced) elements.
  size_t get_misplaced_elements_array_size() const {
    return unknown_legal_elements_array_.size();
  }
  const ElementPtr& get_misplaced_elements_array_at(size_t i) const {
    return unknown_legal_elements_array_[i];
  }

  // Add the given set of attributes to the element's unknown attributes.
  // Element takes ownership of attributes.
  void AddUnknownAttributes(kmlbase::Attributes* attributes);

  // This returns a pointer to the Attributes class holding all unknown
  // attributes for this element found during parse.  This returns NULL if
  // there are no unparsed attributes.  Ownership of the object is retained
  // by the Element class.
  const kmlbase::Attributes* GetUnknownAttributes() const {
    return unknown_attributes_.get();
  }

  // This is the set of xmlns:PREFIX=NAMESPACE attributes on the
  // element if any.  The attribute keys are without the "xmlns:" prefix.
  // The default namespace is merely an "unknown" attribute
  // of "xmlns" in the normal "unknown" attributes list.  Use
  // get_default_xmlns() to access the default namespace for an element.
  const kmlbase::Attributes* GetXmlns() const {
    return xmlns_.get();
  }

  // This merges in the given set of prefix/namespace attributes into the
  // the xml namespaces set for this element.  Each prefix is is _just_ the
  // namespace prefix.  Each prefix added here appears in the
  // SerializeAttributeswith a "xmlns:" prepended.
  void MergeXmlns(const kmlbase::Attributes& xmlns);

  // Permits polymorphic use of Field methods.
  virtual bool SetBool(bool* val) { return false; }
  virtual bool SetDouble(double* val) { return false; }
  virtual bool SetInt(int* val) { return false; }
  virtual bool SetEnum(int* val) { return false; }
  virtual bool SetString(string* val) { return false; }

  // Accepts the visitor for this element (this must be overridden for each
  // element type).
  // TODO(dbeaumont): Make pure virtual when all sub-classes implement Accept().
  virtual void Accept(Visitor* visitor);

  // This needs to be implemented by subclasses with child elements and must
  // call its parent's implementation first. The default implementation does
  // nothing.
  virtual void AcceptChildren(VisitorDriver* driver) {
    /* Inlinable for efficiency */
  }

 protected:
  // Element is an abstract base class and is never created directly.
  Element();
  Element(KmlDomType type_id);

  // This sets the given complex child to a field of this element.
  // The intended usage is to implement the set_child() and clear_child()
  // methods in a concrete element.
  template <class T>
  bool SetComplexChild(const T& child, T* field) {
    if (child == NULL) {
      // TODO: remove child and children from ID maps...
      *field = NULL;  // Assign removes reference and possibly deletes Element.
      return true;
    } else if (child->SetParent(this)) {
      *field = child;  // This first releases the reference to previous field.
      return true;
    }
    return false;
  }

  // This adds the given complex child to an array in this element.
  template <class T>
  bool AddComplexChild(const T& child, std::vector<T>* vec) {
    // NULL child ignored.
    if (child && child->SetParent(this)) {
      vec->push_back(child);
      return true;
    }
    return false;
  }

  // Allows subclasses to easily visit repeated fields.
  template <class T>
  static void AcceptRepeated(std::vector<T>* elements, VisitorDriver* driver) {
    typename std::vector<T>::iterator it;
    for (it = elements->begin(); it != elements->end(); ++it) {
      driver->Visit(*it);
    }
  }

  // This is the internal implementation of the various DeleteTAt() methods.
  template <class T>
  static T DeleteFromArrayAt(std::vector<T>* array, size_t i) {
    if (!array || i >= array->size()) {
      return NULL;
    }
    T e = (*array)[i];
    array->erase(array->begin() + i);
    // TODO: notify e's XmlFile about the delete (kmlengine::KmlFile, for
    // example would want to remove e from its internal maps).
    // TODO: disparent e
    return e;
  }

 private:
  KmlDomType type_id_;
  string char_data_;
  // A vector of strings to contain unknown non-KML elements discovered during
  // parse.
  std::vector<string> unknown_elements_array_;
  // A vector of Element*'s to contain known KML elements found during parse
  // to be in illegal positions, e.g. <Placemark><Document>.
  std::vector<ElementPtr> unknown_legal_elements_array_;
  // Unknown attributes found during parse are copied out and a pointer is
  // stored. The object is dynamically allocated so every element is not
  // burdened with an unnecessary Attributes object.
  boost::scoped_ptr<kmlbase::Attributes> unknown_attributes_;
  // Any Element may have 0 or more xmlns attributes.
  boost::scoped_ptr<kmlbase::Attributes> xmlns_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Element);
};

// This class implements common code for use in serializing most elements.
// Intended usage is as follows:
// ConcreteElement::Serialize(Serializer& serializer) const {
//   ElementSerializer element_serializer(Type(), serializer);
//   // serialize each child element and/or field
//   // ElementSerializer dtor ends serialization properly.
// }
class ElementSerializer {
 public:
  ElementSerializer(const Element& element, Serializer& serializer);
  ~ElementSerializer();

 private:
  const Element& element_;
  Serializer& serializer_;
};

// This class template is essentially common code for all elements based
// directly on Element.
template<int I>
class BasicElement : public Element {
 public:
  // This static method makes the class useable with ElementCast.
  static KmlDomType ElementType() { return static_cast<KmlDomType>(I); }
  virtual KmlDomType Type() const { return ElementType(); }
  virtual bool IsA(KmlDomType type) const { return type == ElementType(); }
};

// A field is generally short lived and holds the element id and character data
// for that field during parse.  When a Field is presented to AddElement() and
// is recognized by a parent element that parent typically copies the value of
// the Field to a field held within the parent.  However, when a "misplaced"
// field is parsed it is held in this form in Element's misplaced elements
// list.  Known child fields are serialized by their parents, but a Serialize
// method implementation is provided specifically to provide a means to
// serialize a Field held in an Element's misplaced elements list.  For
// example, <snippet> is a known element and is parsed initially into a Field,
// but since no element accepts <snippet> this results in a <snippet> Field in
// the parent element's misplaced elements list.
class Field : public Element {
 public:
  Field(KmlDomType type_id);

  // Serialize this Field to the given serializer.  See the class comment above
  // for when this is used.
  virtual void Serialize(Serializer& serialize) const;

  // Sets the given bool from the character data.  If no val pointer is
  // supplied false is returned, else true is returned and the val is set.
  bool SetBool(bool* val);

  // Sets the given double from the character data.  If no val pointer is
  // supplied false is returned, else true is returned and the val is set.
  bool SetDouble(double *val);

  // Sets the given int from the character data.  If no val pointer is
  // supplied false is returned, else true is returned and the val is set.
  bool SetInt(int* val);

  // Sets the given enum from the character data.  If no val pointer is
  // supplied false is returned, else true is returned and the val is set.
  bool SetEnum(int* enum_val);

  // Sets the given string from the character data.  If no val pointer is
  // supplied false is returned, else true is returned and the val is set.
  bool SetString(string* val);

 private:
  const Xsd& xsd_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Field);
};

}  // namespace kmldom

#endif  // KML_DOM_ELEMENT_H__
