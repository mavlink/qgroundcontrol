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

// This file contains the declaration of the internal XmlSerializer class.
// NOTE: This class is internal to libkml and is not intended for use in
// client code outside libkml.

#ifndef KML_DOM_XML_SERIALIZER_H__
#define KML_DOM_XML_SERIALIZER_H__

#include <ostream>
#include <stack>
#include <vector>
#include "kml/base/attributes.h"
#include "kml/base/vec3.h"
#include "kml/dom/serializer.h"
#include "kml/dom/xsd.h"
#include "kml/dom.h"

namespace kmldom {

// A poor man's std::ostringstream.  Note that this uses a non-namespace
// qualified string.  See base/util.h for more info about string vs std::string.
class StringAdapter {
 public:
  StringAdapter(string* str)
    : str_(str) {
  }

  void write(const char* s, size_t n) {
    str_->append(s, n);
  }

  void put(char c) {
    str_->append(1, c);
  }
 private:
  string* str_;
};

// T should match this signature:
// class T {
//  public:
//   void write(const char*, size_t);
//   void put(char c);
// };
// C++ std::ostream matches T
template<class T>
class XmlSerializer : public Serializer {
 public:
  static void Serialize(const ElementPtr& root, const char* newline,
                        const char* indent, T* output) {
   if (!root || !newline || !indent || !output) {
     return;
   }
   boost::scoped_ptr<XmlSerializer> xml_ostream_serializer(
       new XmlSerializer(newline, indent, output));
   root->Serialize(*xml_ostream_serializer);
 }

  // Construct a serializer with the given strings for line breaks and
  // indentation.  The indent string is used once for each level of
  // indentation.  For no line break and/or indent whitespace use "".  This is
  // primarily for unit testing.  Use SerializePrettyToBase whenever
  // possible.  Use kmldom::SerializeToBase() external client code.
  XmlSerializer(const char* newline, const char* indent, T* output)
    : newline_(newline),
      indent_(indent),
      output_(output),
      start_pending_(false) {
  }

  virtual ~XmlSerializer() {}

  // Emit the start tag of the given element: <Placemark id="pm123">.
  virtual void BeginById(int type_id, const kmlbase::Attributes& attributes) {
    // Here we just record the element we're starting and its attributes if
    // it has any.  The "<TAGNAME [name="VAL" ...]..." are not emmited until
    // it is known if this is a nil element or not.
    EmitStart(false);
    Indent();
    tag_stack_.push(type_id);  // So we know what tag to use in End().
    if (attributes.GetSize() > 0) {
      // TODO: Attributes::SerializeToBase would be handy.
      attributes.Serialize(&serialized_attributes_);
    }
    start_pending_ = true;
  }

  // Emit the end tag of the given element: </Placemark>.
  virtual void End() {
    int type_id = tag_stack_.top();
    // TODO: make this less fiddly
    if (EmitStart(true)) {
      tag_stack_.pop();
    } else {
      tag_stack_.pop();
      Indent();
      output_->write("</", 2);
      const string& tag_name = xsd_.ElementName(type_id);
      output_->write(tag_name.data(), tag_name.size());
      output_->put('>');
      Newline();
    }
  }

  // Emit the XML for the field of the given type with the given content
  // as its character data.  If value is empty a nil element is emitted.
  virtual void SaveStringFieldById(int type_id, string value) {
    EmitStart(false);
    Indent();
    const string& tag_name = xsd_.ElementName(type_id);
    output_->put('<');
    output_->write(tag_name.data(), tag_name.size());
    if (value.empty()) {  // Special case to emit <TAGNAME/>
      output_->put('/');
    } else {  // <TAGNAME>VALUE</TAGNAME>
      output_->put('>');
      WriteQuoted(value);
      output_->write("</", 2);
      output_->write(tag_name.data(), tag_name.size());
    }
    output_->put('>');
    Newline();
  }

  // Save out character data.
  virtual void SaveContent(const string& content, bool maybe_quote) {
    EmitStart(false);
    if (maybe_quote) {
      WriteQuoted(content);
    } else {
      output_->write(content.data(), content.size());
    }
  }

  // Save a lon,lat,alt tuple as appears within <coordinates>.
  virtual void SaveVec3(const kmlbase::Vec3& vec3) {
    EmitStart(false);
    Indent();
    string val = kmlbase::ToString(vec3.get_longitude());
    output_->write(val.data(), val.size());
    output_->put(',');
    val = kmlbase::ToString(vec3.get_latitude());
    output_->write(val.data(), val.size());
    // Ideally, we'd only emit if vec3.has_altitude(), but lots of test cases
    // expect lon,lat,0
    output_->put(',');
    val = kmlbase::ToString(vec3.get_altitude());
    output_->write(val.data(), val.size());
    // In libkml 1.2 a "\n" was baked into Serializer::SaveVec3.  We emit an
    // explicit "\n" for compatibility instead of calling Newline() because
    // Newline() could be an empty string which would effectively concatenate
    // coordinates items in SerializeRaw.
    if (newline_.empty()) {
      output_->write("\n", 1);
    } else {
      Newline();
    }
  }

  // Save a Color32 value as its AABBGGRR representation.
  virtual void SaveColor(int type_id, const kmlbase::Color32& color) {
    EmitStart(false);
    SaveFieldById(type_id, color.to_string_abgr());
  }

  // Emit one level of indentation.
  virtual void Indent() {
    if (!indent_.empty()) {
      size_t depth = tag_stack_.size();
      while (depth--) {
        output_->write(indent_.data(), indent_.size());
      }
    }
  }

 private:
  // Emit a line break.
  void Newline() {
    if (!newline_.empty()) {
      output_->write(newline_.data(), newline_.size());
    }
  }

  // Emit quoted. See Serializer::MaybeQuoteString().
  void WriteQuoted(const string& value) {
    string quoted = MaybeQuoteString(value);
    output_->write(quoted.data(), quoted.size());
  }

  bool EmitStart(bool is_nil) {
    if (!start_pending_) {
      return false;
    }
    output_->put('<');
    const string& tag_name = xsd_.ElementName(tag_stack_.top());
    output_->write(tag_name.data(), tag_name.size());
    if (!serialized_attributes_.empty()) {
      output_->write(serialized_attributes_.data(),
                     serialized_attributes_.size());
      serialized_attributes_.clear();
    }
    if (is_nil) {
      output_->write("/>", 2);
    } else {
      output_->put('>');
    }
    Newline();
    start_pending_ = false;
    return true;
  }

  const string newline_;
  const string indent_;
  T* output_;
  std::stack<int> tag_stack_;
  bool start_pending_;
  string serialized_attributes_;
};

}  // end namespace kmldom

#endif  // KML_DOM_XML_SERIALIZER_H__
