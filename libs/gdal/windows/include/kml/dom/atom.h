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

// This file contains the declaration of Atom elements used in KML.
// See: http://atompub.org/rfc4287.html.

#ifndef KML_DOM_ATOM_H__
#define KML_DOM_ATOM_H__

#include "kml/dom/element.h"

namespace kmldom {

// <atom:author>, RFC 4287 4.2.1, and 3.2 (atomPersonConstruct)
class AtomAuthor : public BasicElement<Type_AtomAuthor> {
 public:
  virtual ~AtomAuthor();

  // <atom:name>
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

  // <atom:uri>, RFC 3987
  const string& get_uri() const { return uri_; }
  bool has_uri() const { return has_uri_; }
  void set_uri(const string& value) {
    uri_ = value;
    has_uri_ = true;
  }
  void clear_uri() {
    uri_.clear();
    has_uri_ = false;
  }

  // <atom:email>, RFC 2822
  const string& get_email() const { return email_; }
  bool has_email() const { return has_email_; }
  void set_email(const string& value) {
    email_ = value;
    has_email_ = true;
  }
  void clear_email() {
    email_.clear();
    has_email_ = false;
  }

 private:
  bool has_name_;
  string name_;
  bool has_uri_;
  string uri_;
  bool has_email_;
  string email_;
  friend class KmlFactory;
  AtomAuthor();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AtomAuthor);
};

// Elements common to <atom:feed> and <atom:entry>.
class AtomCommon : public Element {
 public:
  // <atom:id>
  const string& get_id() const { return id_; }
  bool has_id() const { return has_id_; }
  void set_id(const string& value) {
    id_ = value;
    has_id_ = true;
  }
  void clear_id() {
    id_.clear();
    has_id_ = false;
  }

  // <atom:title>
  const string& get_title() const { return title_; }
  bool has_title() const { return has_title_; }
  void set_title(const string& value) {
    title_ = value;
    has_title_ = true;
  }
  void clear_title() {
    title_.clear();
    has_title_ = false;
  }

  // <atom:updated>
  const string& get_updated() const { return updated_; }
  bool has_updated() const { return has_updated_; }
  void set_updated(const string& value) {
    updated_ = value;
    has_updated_ = true;
  }
  void clear_updated() {
    updated_.clear();
    has_updated_ = false;
  }

  // <atom:category>...
  void add_category(const AtomCategoryPtr& entry);
  size_t get_category_array_size() const {
    return category_array_.size();
  }
  const AtomCategoryPtr& get_category_array_at(size_t index) const {
    return category_array_[index];
  }

  // <atom:link>...
  void add_link(const AtomLinkPtr& entry);
  size_t get_link_array_size() const {
    return link_array_.size();
  }
  const AtomLinkPtr& get_link_array_at(size_t index) const {
    return link_array_[index];
  }

 protected:
  AtomCommon();
  void AddElement(const ElementPtr& element);
  virtual void Serialize(Serializer& serializer) const;

 private:
  friend class KmlFactory;
  friend class KmlHandler;
  friend class Serializer;
  bool has_id_;
  string id_;
  bool has_title_;
  string title_;
  bool has_updated_;
  string updated_;
  std::vector<AtomCategoryPtr> category_array_;
  std::vector<AtomLinkPtr> link_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AtomCommon);
};

// <atom:category scheme="..." term="..." label=..."/>, RFC 4287 4.2.2
// NOTE: This element is not part of the OGC KML 2.2 standard.
class AtomCategory : public BasicElement<Type_AtomCategory> {
 public:
  virtual ~AtomCategory();

  // term=
  const string& get_term() const { return term_; }
  bool has_term() const { return has_term_; }
  void set_term(const string& value) {
    term_ = value;
    has_term_ = true;
  }
  void clear_term() {
    term_.clear();
    has_term_ = false;
  }

  // scheme=
  const string& get_scheme() const { return scheme_; }
  bool has_scheme() const { return has_scheme_; }
  void set_scheme(const string& value) {
    scheme_ = value;
    has_scheme_ = true;
  }
  void clear_scheme() {
    scheme_.clear();
    has_scheme_ = false;
  }

  // label=
  const string& get_label() const { return label_; }
  bool has_label() const { return has_label_; }
  void set_label(const string& value) {
    label_ = value;
    has_label_ = true;
  }
  void clear_label() {
    label_.clear();
    has_label_ = false;
  }

 private:
  friend class KmlFactory;
  AtomCategory();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  virtual void ParseAttributes(kmlbase::Attributes* attributes);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  virtual void SerializeAttributes(kmlbase::Attributes* attributes) const;
  bool has_term_;
  string term_;
  bool has_scheme_;
  string scheme_;
  bool has_label_;
  string label_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AtomCategory);
};

// <atom:content src="..."  type="...">, RFC 4287 4.1.3
// NOTE: This element is not part of the OGC KML 2.2 standard.
class AtomContent : public BasicElement<Type_AtomContent> {
 public:
  virtual ~AtomContent();

  // src=
  const string& get_src() const { return src_; }
  bool has_src() const { return has_src_; }
  void set_src(const string& value) {
    src_ = value;
    has_src_ = true;
  }
  void clear_src() {
    src_.clear();
    has_src_ = false;
  }

  // type=
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

 private:
  friend class KmlFactory;
  AtomContent();
  friend class KmlHandler;
  void ParseAttributes(kmlbase::Attributes* attributes);
  void SerializeAttributes(kmlbase::Attributes* attributes) const;
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  bool has_src_;
  string src_;
  bool has_type_;
  string type_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AtomContent);
};

// <atom:entry>, RFC 4287 4.1.2
// NOTE: This element is not part of the OGC KML 2.2 standard.
class AtomEntry : public AtomCommon {
 public:
  virtual ~AtomEntry();
  virtual KmlDomType Type() const { return Type_AtomEntry; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_AtomEntry;
  }
  // This static method makes the class useable with ElementCast.
  static KmlDomType ElementType() {
    return static_cast<KmlDomType>(Type_AtomEntry);
  }

  // <atom:summary>
  const string& get_summary() const { return summary_; }
  bool has_summary() const { return has_summary_; }
  void set_summary(const string& value) {
    summary_ = value;
    has_summary_ = true;
  }
  void clear_summary() {
    summary_.clear();
    has_summary_ = false;
  }

  // <atom:content>
  const AtomContentPtr& get_content() const { return content_; }
  bool has_content() const { return content_ != NULL; }
  void set_content(const AtomContentPtr& content) {
    SetComplexChild(content, &content_);
  }
  void clear_content() {
    set_content(NULL);
  }

 private:
  friend class KmlFactory;
  AtomEntry();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  bool has_summary_;
  string summary_;
  AtomContentPtr content_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AtomEntry);
};

// <atom:feed>, RFC 4287 4.1.1
// NOTE: This element is not part of the OGC KML 2.2 standard.
class AtomFeed : public AtomCommon {
 public:
  virtual ~AtomFeed();
  virtual KmlDomType Type() const { return Type_AtomFeed; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_AtomFeed;
  }
  // This static method makes the class useable with ElementCast.
  static KmlDomType ElementType() {
    return static_cast<KmlDomType>(Type_AtomFeed);
  }

  // <atom:entry>...
  void add_entry(const AtomEntryPtr& entry);
  size_t get_entry_array_size() const {
    return entry_array_.size();
  }
  const AtomEntryPtr& get_entry_array_at(size_t index) const {
    return entry_array_[index];
  }

 private:
  friend class KmlFactory;
  AtomFeed();
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  std::vector<AtomEntryPtr> entry_array_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AtomFeed);
};

// <atom:link>, RFC 4287 4.2.7
class AtomLink : public BasicElement<Type_AtomLink> {
 public:
  virtual ~AtomLink();

  // href=, RFC 4287 4.2.7.1, RFC 3987
  const string& get_href() const { return href_; }
  bool has_href() const { return has_href_; }
  void set_href(const string& value) {
    href_ = value;
    has_href_ = true;
  }
  void clear_href() {
    href_.clear();
    has_href_ = false;
  }

  // rel=, RFC 4287 4.2.7.2, RFC 3987
  const string& get_rel() const { return rel_; }
  bool has_rel() const { return has_rel_; }
  void set_rel(const string& value) {
    rel_ = value;
    has_rel_ = true;
  }
  void clear_rel() {
    rel_.clear();
    has_rel_ = false;
  }

  // type=, RFC 4287 4.2.7.3, MIME
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

  // hreflang=, RFC 4287 4.2.7.4, RFC 3066
  const string& get_hreflang() const { return hreflang_; }
  bool has_hreflang() const { return has_hreflang_; }
  void set_hreflang(const string& value) {
    hreflang_ = value;
    has_hreflang_ = true;
  }
  void clear_hreflang() {
    hreflang_.clear();
    has_hreflang_ = false;
  }

  // title=, RFC 4287 4.2.7.5
  const string& get_title() const { return title_; }
  bool has_title() const { return has_title_; }
  void set_title(const string& value) {
    title_ = value;
    has_title_ = true;
  }
  void clear_title() {
    title_.clear();
    has_title_ = false;
  }

  // length=, RFC 4287 4.2.7.6
  int get_length() const { return length_; }
  bool has_length() const { return has_length_; }
  void set_length(const int value) {
    length_ = value;
    has_length_ = true;
  }
  void clear_length() {
    length_ = 0;
    has_length_ = false;
  }

 private:
  bool has_href_;
  string href_;
  bool has_rel_;
  string rel_;
  bool has_type_;
  string type_;
  bool has_hreflang_;
  string hreflang_;
  bool has_title_;
  string title_;
  bool has_length_;
  int length_;
  friend class KmlFactory;
  AtomLink();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  void ParseAttributes(kmlbase::Attributes* attributes);
  void SerializeAttributes(kmlbase::Attributes* attributes) const;
  friend class Serializer;
  virtual void Serialize(Serializer& serializer) const;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(AtomLink);
};

}  // end namespace kmldom

#endif  // KML_DOM_ATOM_H__
