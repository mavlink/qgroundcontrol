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

#ifndef KML_DOM_SNIPPET_H__
#define KML_DOM_SNIPPET_H__

#include "kml/dom/element.h"
#include "kml/dom/kml22.h"
#include "kml/base/util.h"

namespace kmlbase {
class Attributes;
}

namespace kmldom {

class Serializer;
class Visitor;

// This is SnippetType in the KML standard.
class SnippetCommon : public Element {
 public:
  virtual ~SnippetCommon();
  virtual KmlDomType Type() const { return Type_Snippet; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Snippet;
  }

  // This is the character data content of <Snippet>
  const string& get_text() const { return text_; }
  bool has_text() const { return has_text_; }
  void set_text(const string& value) {
    text_ = value;
    has_text_ = true;
  }
  void clear_text() {
    text_.clear();
    has_text_ = false;
  }

  // maxlines=
  int get_maxlines() const { return maxlines_; }
  bool has_maxlines() const { return has_maxlines_; }
  void set_maxlines(int value) {
    maxlines_ = value;
    has_maxlines_ = true;
  }
  void clear_maxlines() {
    maxlines_ = 2;
    has_maxlines_ = false;
  }

 protected:
  SnippetCommon();
  virtual void AddElement(const ElementPtr& child);
  virtual void ParseAttributes(kmlbase::Attributes* attributes);
  virtual void Serialize(Serializer& serializer) const;
  virtual void SerializeAttributes(kmlbase::Attributes* attributes) const;

 private:
  string text_;
  bool has_text_;
  int maxlines_;
  bool has_maxlines_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(SnippetCommon);
};

// <Snippet>
class Snippet : public SnippetCommon {
 public:
  virtual ~Snippet();
  virtual KmlDomType Type() const { return Type_Snippet; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_Snippet;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  Snippet();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Snippet);
};

// <linkSnippet>
class LinkSnippet : public SnippetCommon {
 public:
  virtual ~LinkSnippet();
  virtual KmlDomType Type() const { return Type_linkSnippet; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_linkSnippet;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  LinkSnippet();
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(LinkSnippet);
};

}  // end namespace kmldom

#endif  // KML_DOM_SNIPPET_H__
