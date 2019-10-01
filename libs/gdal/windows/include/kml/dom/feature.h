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

// This file contains the declaration of the abstract Feature element.

#ifndef KML_DOM_FEATURE_H__
#define KML_DOM_FEATURE_H__

#include "kml/dom/abstractview.h"
#include "kml/dom/atom.h"
#include "kml/dom/extendeddata.h"
#include "kml/dom/kml22.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/object.h"
#include "kml/dom/region.h"
#include "kml/dom/snippet.h"
#include "kml/dom/styleselector.h"
#include "kml/dom/timeprimitive.h"
#include "kml/dom/xal.h"
#include "kml/base/util.h"

namespace kmldom {

class VisitorDriver;

// OGC KML 2.2 Standard: 9.1 kml:AbstractFeatureGroup
// OGC KML 2.2 XSD: <element name="AbstractFeatureGroup"...
class Feature : public Object {
 public:
  virtual ~Feature();
  virtual KmlDomType Type() const { return Type_Feature; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Feature || Object::IsA(type);
  }

  // <name>
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

  // <visibility>
  bool get_visibility() const { return visibility_; }
  bool has_visibility() const { return has_visibility_; }
  void set_visibility(bool value) {
    visibility_ = value;
    has_visibility_ = true;
  }
  void clear_visibility() {
    visibility_ = true;  // Default <visibility> is true.
    has_visibility_ = false;
  }

  // <open>
  bool get_open() const { return open_; }
  bool has_open() const { return has_open_; }
  void set_open(bool value) {
    open_ = value;
    has_open_ = true;
  }
  void clear_open() {
    open_ = false;
    has_open_ = false;
  }

  // <atom:author>
  const AtomAuthorPtr& get_atomauthor() const { return atomauthor_; }
  bool has_atomauthor() const { return atomauthor_ != NULL; }
  void set_atomauthor(const AtomAuthorPtr& atomauthor) {
    SetComplexChild(atomauthor, &atomauthor_);
  }
  void clear_atomauthor() {
    set_atomauthor(NULL);
  }

  // <atom:link>
  const AtomLinkPtr& get_atomlink() const { return atomlink_; }
  bool has_atomlink() const { return atomlink_ != NULL; }
  void set_atomlink(const AtomLinkPtr& atomlink) {
    SetComplexChild(atomlink, &atomlink_);
  }
  void clear_atomlink() {
    set_atomlink(NULL);
  }

  // <address>
  const string& get_address() const { return address_; }
  bool has_address() const { return has_address_; }
  void set_address(const string& value) {
    address_ = value;
    has_address_ = true;
  }
  void clear_address() {
    address_.clear();
    has_address_ = false;
  }

  // <xal:AddressDetails>
  const XalAddressDetailsPtr& get_xaladdressdetails() const {
    return xaladdressdetails_;
  }
  bool has_xaladdressdetails() const { return xaladdressdetails_ != NULL; }
  void set_xaladdressdetails(const XalAddressDetailsPtr& xaladdressdetails) {
    SetComplexChild(xaladdressdetails, &xaladdressdetails_);
  }
  void clear_xaladdressdetails() {
    set_xaladdressdetails(NULL);
  }

  // <phoneNumber>
  const string& get_phonenumber() const { return phonenumber_; }
  bool has_phonenumber() const { return has_phonenumber_; }
  void set_phonenumber(const string& value) {
    phonenumber_ = value;
    has_phonenumber_ = true;
  }
  void clear_phonenumber() {
    phonenumber_.clear();
    has_phonenumber_ = false;
  }

  // TODO: "little" <snippet> (presently preserved as a misplaced child)
  // <Snippet>
  const SnippetPtr& get_snippet() const { return snippet_; }
  bool has_snippet() const { return snippet_ != NULL; }
  void set_snippet(const SnippetPtr& snippet) {
    SetComplexChild(snippet, &snippet_);
  }
  void clear_snippet() {
    set_snippet(NULL);
  }

  // <description>
  const string& get_description() const { return description_; }
  bool has_description() const { return has_description_; }
  void set_description(const string& value) {
    description_ = value;
    has_description_ = true;
  }
  void clear_description() {
    description_.clear();
    has_description_ = false;
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

  // TimePrimitive
  const TimePrimitivePtr& get_timeprimitive() const { return timeprimitive_; }
  bool has_timeprimitive() const { return timeprimitive_ != NULL; }
  void set_timeprimitive(const TimePrimitivePtr& timeprimitive) {
    SetComplexChild(timeprimitive, &timeprimitive_);
  }
  void clear_timeprimitive() {
    set_timeprimitive(NULL);
  }

  // <styleUrl>
  const string& get_styleurl() const { return styleurl_; }
  string& styleurl() { return styleurl_; }
  bool has_styleurl() const { return has_styleurl_; }
  void set_styleurl(const string& value) {
    styleurl_ = value;
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

  // <Region>
  const RegionPtr& get_region() const { return region_; }
  bool has_region() const { return region_ != NULL; }
  void set_region(const RegionPtr& region) {
    SetComplexChild(region, &region_);
  }
  void clear_region() {
    set_region(NULL);
  }

  // TODO: <Metadata> (presently preserved as a misplaced child)
  // <ExtendedData>
  const ExtendedDataPtr& get_extendeddata() const { return extendeddata_; }
  bool has_extendeddata() const { return extendeddata_ != NULL; }
  void set_extendeddata(const ExtendedDataPtr& extendeddata) {
    SetComplexChild(extendeddata, &extendeddata_);
  }
  void clear_extendeddata() {
    set_extendeddata(NULL);
  }

  // From kml:AbstractFeatureSimpleExtensionGroup.

  // <gx:balloonVisibility>
  bool get_gx_balloonvisibility() const { return gx_balloonvisibility_; }
  bool has_gx_balloonvisibility() const { return has_gx_balloonvisibility_; }
  void set_gx_balloonvisibility(bool value) {
    gx_balloonvisibility_ = value;
    has_gx_balloonvisibility_ = true;
  }
  void clear_gx_balloonvisibility() {
    gx_balloonvisibility_ = false;
    has_gx_balloonvisibility_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void AcceptChildren(VisitorDriver* driver);

 protected:
  // Feature is abstract.
  Feature();
  virtual void AddElement(const ElementPtr& element);
  void SerializeBeforeStyleSelector(Serializer& serialize) const;
  void SerializeAfterStyleSelector(Serializer& serialize) const;
  virtual void Serialize(Serializer& serialize) const;

 private:
  string name_;
  bool has_name_;
  bool visibility_;
  bool has_visibility_;
  bool open_;
  bool has_open_;
  AtomAuthorPtr atomauthor_;
  AtomLinkPtr atomlink_;
  string address_;
  bool has_address_;
  XalAddressDetailsPtr xaladdressdetails_;
  string phonenumber_;
  bool has_phonenumber_;
  SnippetPtr snippet_;
  string description_;
  bool has_description_;
  AbstractViewPtr abstractview_;
  TimePrimitivePtr timeprimitive_;
  string styleurl_;
  bool has_styleurl_;
  StyleSelectorPtr styleselector_;
  RegionPtr region_;
  ExtendedDataPtr extendeddata_;
  bool gx_balloonvisibility_;
  bool has_gx_balloonvisibility_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Feature);
};

}  // namespace kmldom

#endif  // KML_DOM_FEATURE_H__
