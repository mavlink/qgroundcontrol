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

// This file contains the declaration of the BalloonStyle element.

#ifndef KML_DOM_BALLOONSTYLE_H__
#define KML_DOM_BALLOONSTYLE_H__

#include "kml/base/color32.h"
#include "kml/dom/substyle.h"
#include "kml/dom/kml22.h"

namespace kmldom {

class Visitor;

class BalloonStyle : public SubStyle {
 public:
  virtual ~BalloonStyle();
  virtual KmlDomType Type() const { return Type_BalloonStyle; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_BalloonStyle || SubStyle::IsA(type);
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

  // <textColor>
  const kmlbase::Color32& get_textcolor() const {
    return textcolor_;
  }
  bool has_textcolor() const {
    return has_textcolor_;
  }
  void set_textcolor(const kmlbase::Color32& textcolor) {
    textcolor_ = textcolor;
    has_textcolor_ = true;
  }
  void clear_textcolor() {
    textcolor_ = kmlbase::Color32(0xff000000);
    has_textcolor_ = false;
  }

  // <text>
  const string& get_text() const {
    return text_;
  }
  bool has_text() const {
    return has_text_;
  }
  void set_text(const string& text) {
    text_ = text;
    has_text_ = true;
  }
  void clear_text() {
    text_.clear();
    has_text_ = false;
  }

  // <displayMode>
  int get_displaymode() const {
    return displaymode_;
  }
  bool has_displaymode() const {
    return has_displaymode_;
  }
  void set_displaymode(int displaymode) {
    displaymode_ = displaymode;
    has_displaymode_ = true;
  }
  void clear_displaymode() {
    displaymode_ = DISPLAYMODE_DEFAULT;
    has_displaymode_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 private:
  friend class KmlFactory;
  BalloonStyle();
  friend class KmlHandler;
  virtual void AddElement(const ElementPtr& element);
  friend class Serializer;
  virtual void Serialize(Serializer& serialize) const;
  kmlbase::Color32 bgcolor_;
  bool has_bgcolor_;
  kmlbase::Color32 textcolor_;
  bool has_textcolor_;
  string text_;
  bool has_text_;
  int displaymode_;
  bool has_displaymode_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(BalloonStyle);
};

}  // end namespace kmldom

#endif // KML_DOM_BALLOONSTYLE_H__
