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

// This file contains the declaration of the internal Serializer class.

#ifndef KML_DOM_SERIALIZER_H__
#define KML_DOM_SERIALIZER_H__

#include <sstream>
#include "kml/base/string_util.h"
#include "kml/dom/kml_ptr.h"

namespace kmlbase {
class Attributes;
class Color32;
class Vec3;
}

namespace kmldom {

class Xsd;

// The Serializer class is internal to the KML DOM and is used by each
// Element to save its tag name, fields (attributes and simple elements),
// character data content and/or complex child elements.
class Serializer {
 public:

  Serializer();

  virtual ~Serializer() {}

  // Emit the start tag of the given element: <Placemark id="pm123">.
  virtual void BeginById(int type_id, const kmlbase::Attributes& attributes) {};

  // Emit the end tag of the given element: </Placemark>.
  virtual void End() {};

  // Emit a complex element.
  virtual void SaveElement(const ElementPtr& element);

  // Emit a complex element as a member of the specified group.  For example,
  // when Point is a child of Placemark it is a Geometry, but when it is a
  // child of PhotoOverlay it is emitted with SaveElement and no group id.
  virtual void SaveElementGroup(const ElementPtr& element, int group_id) {
    // Default implementation just calls SaveElement for those serializers
    // that have no need to use the group id of the given child element.
    // This also ensures that a serializer recurses on a complex element
    // whether SaveElement() or SaveElementGroup() is used.
    SaveElement(element);
  }

  // Emit a simple element.
  virtual void SaveStringFieldById(int type_id, string value) {}

  // Save out raw text.  If maybe_quote is true the content is examined
  // for non-XML-valid characters and if so the content is CDATA escaped.
  // If maybe_quote is false the content is emitted directly.
  virtual void SaveContent(const string& content, bool maybe_quote) {};

  // Save a lon,lat,alt tuple as appears within <coordinates>.
  virtual void SaveVec3(const kmlbase::Vec3& vec3);

  // Save a Vec3 with a specified delimiter and with an optional newline char.
  virtual void SaveSimpleVec3(int type_id, const kmlbase::Vec3& vec3,
                              const string& delimiter);

  // Emit indent.
  virtual void Indent() {}

  // Save a Color32 value.
  virtual void SaveColor(int type_id, const kmlbase::Color32& color) {}

  // If value contains any non-XML valid characters a CDATA-escaped
  // string is returned, else the original string is returned.
  const string MaybeQuoteString(const string& value);

  // Save the given value out as the enum element identified by type_id.
  void SaveEnum(int type_id, int enum_value);

  // Save the given value out as the simple element identified by type_id.
  template<typename T>
  void SaveFieldById(int type_id, T value) {
    SaveStringFieldById(type_id, kmlbase::ToString(value));
  }

  // Notify the serializer that an array of the given type of element is being
  // saved.  SaveElement will now be called N times (N == element_count).
  virtual void BeginElementArray(int type_id, size_t element_count) {}

  // Notify the serializer that an array of the given type was just saved.
  virtual void EndElementArray(int type_id) {}

  // This is common code for saving any element array.  The BeginElementArray
  // and EndElementArray methods are called before/after saving all elements.
  template<class T>
  void SaveElementArray(const std::vector<T>& element_array) {
    if (size_t element_count = element_array.size()) {
      BeginElementArray(element_array[0]->Type(), element_count);
      for (size_t i = 0; i < element_count; ++i) {
        SaveElement(element_array[i]);
      }
      EndElementArray(element_array[0]->Type());
    }
  }

  // Notify the serializer that an array of the given group type of element is
  // being saved.  SaveElementGroup will now be called N times (N ==
  // element_count).
  virtual void BeginElementGroupArray(int group_id, size_t element_count) {}

  // Notify the serializer that an array of the given group type was just saved.
  virtual void EndElementGroupArray(int group_id) {}

  // This is common code for saving any substitution group element array.  The
  // BeginElementArray and EndElementArray methods are called before/after
  // saving all elements.
  template<class T>
  void SaveElementGroupArray(const std::vector<T>& element_array,
                             int group_id) {
    if (size_t element_count = element_array.size()) {
      BeginElementGroupArray(group_id, element_count);
      for (size_t i = 0; i < element_count; ++i) {
        SaveElementGroup(element_array[i], group_id);
      }
      EndElementGroupArray(group_id);
    }
  }

 protected:
   const Xsd& xsd_;
};

}  // end namespace kmldom

#endif  // KML_DOM_SERIALIZER_H__
