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

// This file contains the declaration of the <xal:AddressDetails> elements.
// Note, only a subset of XAL using these elements is implemented here.
// However, note that the normal unknown/misplaced element handling of libkml
// is employed thus all of XAL is preserved on parse and emitted on
// serialization.  The portion implemented here pertains to programmatic
// dom access.
//
// xAL complex elements:
// <xal:AddressDetails>
// <xal:AdministrativeArea>
// <xal:Country>
// <xal:Locality>
// <xal:PostalCode>
// <xal:SubAdministrativeArea>
// <xal:Thoroughfare>
//
// xAL simple elements:
// <xal:AdministrativeAreaName>
// <xal:CountryNameCode>
// <xal:LocalityName>
// <xal:PostalCodeNumber>
// <xal:SubAdministrativeAreaName>
// <xal:ThoroughfareName>
// <xal:ThoroughfareNumber>

#ifndef KML_DOM_XAL_H__
#define KML_DOM_XAL_H__

#include "kml/dom/element.h"
#include "kml/base/attributes.h"

namespace kmldom {

// <xal:AddressDetails>
class XalAddressDetails : public BasicElement<Type_XalAddressDetails> {
 public:
  virtual ~XalAddressDetails() {}

  // <xal:Country>
  const XalCountryPtr& get_country() const { return country_; }
  bool has_country() const { return country_ != NULL; }
  void set_country(const XalCountryPtr& country) {
    SetComplexChild(country, &country_);
  }
  void clear_country() { set_country(NULL); }

 private:
  XalAddressDetails();
  XalCountryPtr country_;
  friend class KmlFactory;
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
};

// <xal:AdministrativeArea>
class XalAdministrativeArea : public BasicElement<Type_XalAdministrativeArea> {
 public:
  virtual ~XalAdministrativeArea() {}

  // <xal:AdministrativeAreaName>
  const string& get_administrativeareaname() const {
    return administrativeareaname_;
  }
  bool has_administrativeareaname() const {
    return has_administrativeareaname_;
  }
  void set_administrativeareaname(const string& value) {
    administrativeareaname_ = value;
    has_administrativeareaname_ = true;
  }
  void clear_administrativeareaname() {
    administrativeareaname_.clear();
    has_administrativeareaname_ = false;
  }

  // <xal:Locality>
  const XalLocalityPtr& get_locality() const { return locality_; }
  bool has_locality() const { return locality_ != NULL; }
  void set_locality(const XalLocalityPtr& locality) {
    SetComplexChild(locality, &locality_);
  }

  void clear_locality() { set_locality(NULL); }
  // <xal:SubAdministrativeArea>
  const XalSubAdministrativeAreaPtr& get_subadministrativearea() const {
    return subadministrativearea_;
  }
  bool has_subadministrativearea() const {
    return subadministrativearea_ != NULL;
  }
  void set_subadministrativearea(
      const XalSubAdministrativeAreaPtr& subadministrativearea) {
    SetComplexChild(subadministrativearea, &subadministrativearea_);
  }
  void clear_subadministrativearea() { set_subadministrativearea(NULL); }

 private:
  XalAdministrativeArea();
  bool has_administrativeareaname_;
  string administrativeareaname_;
  XalLocalityPtr locality_;
  XalSubAdministrativeAreaPtr subadministrativearea_;
  friend class KmlFactory;
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
};

// <xal:Country>
class XalCountry : public BasicElement<Type_XalCountry> {
 public:
  virtual ~XalCountry() {}

  // <xal:CountryNameCode>, ISO 3166-1
  const string& get_countrynamecode() const { return countrynamecode_; }
  bool has_countrynamecode() const { return has_countrynamecode_; }
  void set_countrynamecode(const string& value) {
    countrynamecode_ = value;
    has_countrynamecode_ = true;
  }
  void clear_countrynamecode() {
    countrynamecode_.clear();
    has_countrynamecode_ = false;
  }

  // <xal:AdministrativeArea>
  const XalAdministrativeAreaPtr& get_administrativearea() const {
    return administrativearea_;
  }
  bool has_administrativearea() const { return administrativearea_ != NULL; }
  void set_administrativearea(
      const XalAdministrativeAreaPtr& administrativearea) {
    SetComplexChild(administrativearea, &administrativearea_);
  }
  void clear_administrativearea() { set_administrativearea(NULL); }

 private:
  XalCountry();
  bool has_countrynamecode_;
  string countrynamecode_;
  XalAdministrativeAreaPtr administrativearea_;
  friend class KmlFactory;
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(XalCountry);
};

// <xal:Locality>
class XalLocality : public BasicElement<Type_XalLocality> {
 public:
  virtual ~XalLocality() {}

  // <xal:LocalityName>
  const string& get_localityname() const {
    return localityname_;
  }
  bool has_localityname() const {
    return has_localityname_;
  }
  void set_localityname(const string& value) {
    localityname_ = value;
    has_localityname_ = true;
  }
  void clear_localityname() {
    localityname_.clear();
    has_localityname_ = false;
  }

  // <xal:Thoroughfare>
  const XalThoroughfarePtr& get_thoroughfare() const { return thoroughfare_; }
  bool has_thoroughfare() const { return thoroughfare_ != NULL; }
  void set_thoroughfare(const XalThoroughfarePtr& thoroughfare) {
    SetComplexChild(thoroughfare, &thoroughfare_);
  }
  void clear_thoroughfare() { set_thoroughfare(NULL); }

  // <xal:PostalCode>
  const XalPostalCodePtr& get_postalcode() const { return postalcode_; }
  bool has_postalcode() const { return postalcode_ != NULL; }
  void set_postalcode(const XalPostalCodePtr& postalcode) {
    SetComplexChild(postalcode, &postalcode_);
  }
  void clear_postalcode() { set_postalcode(NULL); }

 private:
  XalLocality();
  bool has_localityname_;
  string localityname_;
  XalThoroughfarePtr thoroughfare_;
  XalPostalCodePtr postalcode_;
  friend class KmlFactory;
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
};

// <xal:PostalCode>
class XalPostalCode : public BasicElement<Type_XalPostalCode> {
 public:
  virtual ~XalPostalCode() {}

  // <xal:PostalCodeNumber>
  const string& get_postalcodenumber() const {
    return postalcodenumber_;
  }
  bool has_postalcodenumber() const {
    return has_postalcodenumber_;
  }
  void set_postalcodenumber(const string& value) {
    postalcodenumber_ = value;
    has_postalcodenumber_ = true;
  }
  void clear_postalcodenumber() {
    postalcodenumber_.clear();
    has_postalcodenumber_ = false;
  }

 private:
  XalPostalCode();
  bool has_postalcodenumber_;
  string postalcodenumber_;
  friend class KmlFactory;
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
};

// <xal:SubAdministrativeArea>
class XalSubAdministrativeArea :
    public BasicElement<Type_XalSubAdministrativeArea> {
 public:
  virtual ~XalSubAdministrativeArea() {}

  // <xal:SubAdministrativeAreaName>
  const string& get_subadministrativeareaname() const {
    return subadministrativeareaname_;
  }
  bool has_subadministrativeareaname() const {
    return has_subadministrativeareaname_;
  }
  void set_subadministrativeareaname(const string& value) {
    subadministrativeareaname_ = value;
    has_subadministrativeareaname_ = true;
  }
  void clear_subadministrativeareaname() {
    subadministrativeareaname_.clear();
    has_subadministrativeareaname_ = false;
  }

  // <xal:Locality>
  const XalLocalityPtr& get_locality() const { return locality_; }
  bool has_locality() const { return locality_ != NULL; }
  void set_locality(const XalLocalityPtr& locality) {
    SetComplexChild(locality, &locality_);
  }
  void clear_locality() { set_locality(NULL); }

 private:
  XalSubAdministrativeArea();
  bool has_subadministrativeareaname_;
  string subadministrativeareaname_;
  XalLocalityPtr locality_;
  friend class KmlFactory;
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
};

// <xal:Thoroughfare>
class XalThoroughfare : public BasicElement<Type_XalThoroughfare> {
 public:
  virtual ~XalThoroughfare() {}

  // <xal:ThoroughfareName>
  const string& get_thoroughfarename() const {
    return thoroughfarename_;
  }
  bool has_thoroughfarename() const {
    return has_thoroughfarename_;
  }
  void set_thoroughfarename(const string& value) {
    thoroughfarename_ = value;
    has_thoroughfarename_ = true;
  }
  void clear_thoroughfarename() {
    thoroughfarename_.clear();
    has_thoroughfarename_ = false;
  }

  // <xal:ThoroughfareNumber>
  const string& get_thoroughfarenumber() const {
    return thoroughfarenumber_;
  }
  bool has_thoroughfarenumber() const {
    return has_thoroughfarenumber_;
  }
  void set_thoroughfarenumber(const string& value) {
    thoroughfarenumber_ = value;
    has_thoroughfarenumber_ = true;
  }
  void clear_thoroughfarenumber() {
    thoroughfarenumber_.clear();
    has_thoroughfarenumber_ = false;
  }

 private:
  XalThoroughfare();
  bool has_thoroughfarename_;
  string thoroughfarename_;
  bool has_thoroughfarenumber_;
  string thoroughfarenumber_;
  friend class KmlFactory;
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
};

}  // end namespace kmldom

#endif  // KML_DOM_XAL_H__
