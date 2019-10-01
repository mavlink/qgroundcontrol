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

// This file contains the declaration of the NetworkLinkControl element.

#ifndef KML_DOM_NETWORKLINKCONTROL_H__
#define KML_DOM_NETWORKLINKCONTROL_H__

#include <vector>
#include "kml/dom/abstractview.h"
#include "kml/dom/container.h"
#include "kml/dom/element.h"
#include "kml/dom/feature.h"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/object.h"
#include "kml/base/util.h"

namespace kmldom {

class Visitor;
class VisitorDriver;

// UpdateOperation
// An internal class from which <Create>, <Delete> and <Change> derive. The
// KML XSD uses a choice here which is not readily modeled in C++.
class UpdateOperation : public Element {
 public:
  virtual ~UpdateOperation();

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 protected:
  // UpdateOperation is abstract.
  UpdateOperation();

 private:
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(UpdateOperation);
};

// <Create>
class Create : public UpdateOperation {
 public:
  virtual ~Create();
  virtual KmlDomType Type() const { return ElementType(); }
  virtual bool IsA(KmlDomType type) const { return type == ElementType(); }
  static KmlDomType ElementType() { return Type_Create; }

  // Create targets containers.
  void add_container(const ContainerPtr& container) {
    AddComplexChild(container, &container_array_);
  }

  size_t get_container_array_size() const {
    return container_array_.size();
  }

  const ContainerPtr& get_container_array_at(size_t index) const {
    return container_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Create();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  std::vector<ContainerPtr> container_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Create);
};

// <Delete>
class Delete : public UpdateOperation {
 public:
  virtual ~Delete();
  virtual KmlDomType Type() const { return ElementType(); }
  virtual bool IsA(KmlDomType type) const { return type == ElementType(); }
  static KmlDomType ElementType() { return Type_Delete; }

  // Delete targets Features.
  void add_feature(const FeaturePtr& feature) {
    AddComplexChild(feature, &feature_array_);
  }

  size_t get_feature_array_size() const {
    return feature_array_.size();
  }

  const FeaturePtr& get_feature_array_at(size_t index) const {
    return feature_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Delete();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  std::vector<FeaturePtr> feature_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Delete);
};

// <Change>
class Change : public UpdateOperation {
 public:
  virtual ~Change();
  virtual KmlDomType Type() const { return ElementType(); }
  virtual bool IsA(KmlDomType type) const { return type == ElementType(); }
  static KmlDomType ElementType() { return Type_Change; }

  // Change targets Objects.
  void add_object(const ObjectPtr& object) {
    AddComplexChild(object, &object_array_);
  }

  size_t get_object_array_size() const {
    return object_array_.size();
  }

  const ObjectPtr& get_object_array_at(size_t index) const {
    return object_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Change();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  std::vector<ObjectPtr> object_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Change);
};

// <Update>
class Update : public BasicElement<Type_Update> {
 public:
  virtual ~Update();

  // <targetHref>
  const string& get_targethref() const { return targethref_; }
  bool has_targethref() const { return has_targethref_; }
  void set_targethref(const string& targethref) {
    targethref_ = targethref;
    has_targethref_ = true;
  }
  void clear_targethref() {
    targethref_.clear();
    has_targethref_ = false;
  }

  // <Create>, <Delete> and <Change> elements.
  void add_updateoperation(const UpdateOperationPtr& updateoperation) {
    AddComplexChild(updateoperation, &updateoperation_array_);
  }

  size_t get_updateoperation_array_size() const {
    return updateoperation_array_.size();
  }

  const UpdateOperationPtr& get_updateoperation_array_at(
      size_t index) const {
    return updateoperation_array_[index];
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  Update();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  string targethref_;
  bool has_targethref_;
  std::vector<UpdateOperationPtr> updateoperation_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Update);
};

// <NetworkLinkControl>
class NetworkLinkControl : public BasicElement<Type_NetworkLinkControl> {
 public:
  virtual ~NetworkLinkControl();

  // <minRefreshPeriod>
  double get_minrefreshperiod() const { return minrefreshperiod_; }
  bool has_minrefreshperiod() const { return has_minrefreshperiod_; }
  void set_minrefreshperiod(double value) {
    minrefreshperiod_ = value;
    has_minrefreshperiod_ = true;
  }
  void clear_minrefreshperiod() {
    minrefreshperiod_ = 0.0;
    has_minrefreshperiod_ = false;
  }

  // <maxSessionLength>
  double get_maxsessionlength() const { return maxsessionlength_; }
  bool has_maxsessionlength() const { return has_maxsessionlength_; }
  void set_maxsessionlength(double value) {
    maxsessionlength_ = value;
    has_maxsessionlength_ = true;
  }
  void clear_maxsessionlength() {
    maxsessionlength_ = 0.0;
    has_maxsessionlength_ = false;
  }

  // <cookie>
  const string& get_cookie() const { return cookie_; }
  bool has_cookie() const { return has_cookie_; }
  void set_cookie(const string& cookie) {
    cookie_ = cookie;
    has_cookie_ = true;
  }
  void clear_cookie() {
    cookie_.clear();
    has_cookie_ = false;
  }

  // <message>
  const string& get_message() const { return message_; }
  bool has_message() const { return has_message_; }
  void set_message(const string& message) {
    message_ = message;
    has_message_ = true;
  }
  void clear_message() {
    message_.clear();
    has_message_ = false;
  }

  // <linkName>
  const string& get_linkname() const { return linkname_; }
  bool has_linkname() const { return has_linkname_; }
  void set_linkname(const string& linkname) {
    linkname_ = linkname;
    has_linkname_ = true;
  }
  void clear_linkname() {
    linkname_.clear();
    has_linkname_ = false;
  }

  // <linkDescription>
  const string& get_linkdescription() const { return linkdescription_; }
  bool has_linkdescription() const { return has_linkdescription_; }
  void set_linkdescription(const string& linkdescription) {
    linkdescription_ = linkdescription;
    has_linkdescription_ = true;
  }
  void clear_linkdescription() {
    linkdescription_.clear();
    has_linkdescription_ = false;
  }

  // <linkSnippet>
  const LinkSnippetPtr& get_linksnippet() const { return linksnippet_; }
  bool has_linksnippet() const { return linksnippet_ != NULL; }
  void set_linksnippet(LinkSnippetPtr linksnippet) {
    SetComplexChild(linksnippet, &linksnippet_);
  }
  void clear_linksnippet() {
    set_linksnippet(NULL);
  }

  // <expires>
  const string& get_expires() const { return expires_; }
  bool has_expires() const { return has_expires_; }
  void set_expires(const string& expires) {
    expires_ = expires;
    has_expires_ = true;
  }
  void clear_expires() {
    expires_.clear();
    has_expires_ = false;
  }

  // <Update>
  const UpdatePtr& get_update() const { return update_; }
  bool has_update() const { return update_ != NULL; }
  void set_update(const UpdatePtr& update) {
    SetComplexChild(update, &update_);
  }
  void clear_update() {
    set_update(NULL);
  }

  // AbstractView
  const AbstractViewPtr& get_abstractview() const { return abstractview_; }
  bool has_abstractview() const { return abstractview_ != NULL; }
  void set_abstractview(const AbstractViewPtr& abstractview) {
    SetComplexChild(abstractview, &abstractview_);
  }
  void clear_abstractview() {
    set_abstractview(NULL);
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);
  virtual void AcceptChildren(VisitorDriver* driver);

 private:
  friend class KmlFactory;
  NetworkLinkControl();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  double minrefreshperiod_;
  bool has_minrefreshperiod_;
  double maxsessionlength_;
  bool has_maxsessionlength_;
  string cookie_;
  bool has_cookie_;
  string message_;
  bool has_message_;
  string linkname_;
  bool has_linkname_;
  string linkdescription_;
  bool has_linkdescription_;
  LinkSnippetPtr linksnippet_;
  string expires_;
  bool has_expires_;
  UpdatePtr update_;
  AbstractViewPtr abstractview_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(NetworkLinkControl);
};

}  // namespace kmldom

#endif  // KML_DOM_NETWORKLINKCONTROL_H__
