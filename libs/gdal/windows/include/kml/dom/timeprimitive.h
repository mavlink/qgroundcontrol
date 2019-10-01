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

// This file contains the declarations for the abstract TimePrimitive element
// and the concrete TimeStamp, TimeSpan elements.

#ifndef KML_DOM_TIMEPRIMITIVE_H__
#define KML_DOM_TIMEPRIMITIVE_H__

#include "kml/dom/kml22.h"
#include "kml/dom/object.h"

namespace kmldom {

class Serializer;
class Visitor;

// OGC KML 2.2 Standard: 15.1 kml:AbstractTimePrimitiveGroup
// OGC KML 2.2 XSD: <element name="AbstractTimePrimitiveGroup"...
class TimePrimitive : public Object {
 public:
  virtual ~TimePrimitive();
  virtual KmlDomType Type() const { return Type_TimePrimitive; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_TimePrimitive || Object::IsA(type);
  }

  // Internal API for parse and serialize.
  virtual void AddElement(const ElementPtr& element);

 protected:
  // TimePrimitive is abstract.
  TimePrimitive();

 private:
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(TimePrimitive);
};

// <TimeSpan>
class TimeSpan : public TimePrimitive {
 public:
  virtual ~TimeSpan();
  virtual KmlDomType Type() const { return Type_TimeSpan; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_TimeSpan || TimePrimitive::IsA(type);
  }

  // <begin>
  const string& get_begin() const { return begin_; }
  bool has_begin() const { return has_begin_; }
  void set_begin(const string& value) {
    begin_ = value;
    has_begin_ = true;
  }
  void clear_begin() {
    begin_.clear();
    has_begin_ = false;
  }

  // <end>
  const string& get_end() const { return end_; }
  bool has_end() const { return has_end_; }
  void set_end(const string& value) {
    end_ = value;
    has_end_ = true;
  }
  void clear_end() {
    end_.clear();
    has_end_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 protected:
  TimeSpan();
  virtual void AddElement(const ElementPtr& element);
  virtual void Serialize(Serializer& serializer) const;

 private:
  friend class KmlFactory;
  friend class KmlHandler;
  friend class Serializer;
  string begin_;
  bool has_begin_;
  string end_;
  bool has_end_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(TimeSpan);
};

// <TimeStamp>
class TimeStamp : public TimePrimitive {
 public:
  virtual ~TimeStamp();
  virtual KmlDomType Type() const { return Type_TimeStamp; }
  virtual bool IsA(KmlDomType type) const {
    return type == Type_TimeStamp || TimePrimitive::IsA(type);
  }

  // <when>
  const string& get_when() const { return when_; }
  bool has_when() const { return has_when_; }
  void set_when(const string& value) {
    when_ = value;
    has_when_ = true;
  }
  void clear_when() {
    when_.clear();
    has_when_ = false;
  }

  // Visitor API methods, see visitor.h.
  virtual void Accept(Visitor* visitor);

 protected:
  TimeStamp();
  virtual void AddElement(const ElementPtr& element);
  virtual void Serialize(Serializer& serializer) const;

 private:
  friend class KmlFactory;
  friend class KmlHandler;
  friend class Serializer;
  string when_;
  bool has_when_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(TimeStamp);
};

}  // end namespace kmldom

#endif  // KML_DOM_TIMEPRIMITIVE_H__
