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

// This file contains the declaration of the internal Attributes class.

#ifndef KML_BASE_ATTRIBUTES_H__
#define KML_BASE_ATTRIBUTES_H__

#include <stdlib.h>
#include <map>
#include <sstream>
#include "boost/scoped_ptr.hpp"
#include "kml/base/string_util.h"
#include "kml/base/util.h"

namespace kmlbase {

class Attributes {
 public:
  // Construct the Attributes instance from a list of name-value pairs
  // as is used in expat's startElement.
  static Attributes* Create(const char** attrs);
  static Attributes* Create(const kmlbase::StringVector& attrs);

  // Construct the Attributes instance with no initial name-value pairs.
  Attributes() {}

  // Creates an exact copy of the Attributes object.
  Attributes* Clone() const;

  bool FindValue(const string& key, string* value) const;
  bool FindKey(const string& value, string* key) const;
  size_t GetSize() const {
    return attributes_map_.size();
  }

  // Split prefixed attributes out to a new Attributes.
  Attributes* SplitByPrefix(const string& prefix);

  StringMapIterator CreateIterator() const {
    return StringMapIterator(attributes_map_);
  }

  // Get the value of the given attribute as the templated type.  Returns true
  // if an attribute with this name exits.  If no attribute by this name exists
  // false is returned and the attr_val is untouched.  T can be one of
  // string, int, double or bool.
  template<typename T>
  bool GetValue(const string& attr_name, T* attr_val) const {
    string string_val;
    if (FindValue(attr_name, &string_val)) {
      if (attr_val) {
        FromString(string_val, attr_val);
      }
      return true;
    }
    return false;
  }

  // This is the same as GetValue() + erase().
  template<typename T>
  bool CutValue(const string& attr_name, T* attr_val) {
    if (GetValue(attr_name, attr_val)) {
      attributes_map_.erase(attr_name);
      return true;
    }
    return false;
  }

  // Set the value of the given attribute.  Any previous value for this
  // attribute is overwritten.  T can be one of string, int, double or
  // bool.
  template<typename T>
  void SetValue(const string& attr_name, const T& attr_val) {
    attributes_map_[attr_name] = ToString(attr_val);
  }

  // These are deprecated.  Use Get() and Set().
  // TODO: remove usage elsewhere.
  bool GetString(const string& attr_name, string* attr_val) const {
    return GetValue(attr_name, attr_val);
  }
  bool GetBool(const string& attr_name, bool* attr_val) const {
    return GetValue(attr_name, attr_val);
  }
  bool GetDouble(const string& attr_name, double* attr_val) const {
    return GetValue(attr_name, attr_val);
  }
  void SetString(const string& attr_name, const string& attr_val) {
    SetValue(attr_name, attr_val);
  }

  // Serialize the current state of the Attributes instance into the
  // passed string.  This appends to any content previously in the string.
  // If no string pointer is supplied this method does nothing.
  void Serialize(string* output) const;

  // This sets each attribute from the passed Attributes instance.
  // Any conflicting attributes are overridden from the input.
  void MergeAttributes(const Attributes& attrs);

  // Returns all attribute names.
  // NOTE: This is deprecated.  Use CreateIterator().
  void GetAttrNames(std::vector<string>* attr_names) const;

 private:
  bool Parse(const char** attrs);
  bool Parse(const kmlbase::StringVector& attrs);

  // XML attributes have no order and are unique.  The attribute name is
  // preserved to properly save unknown attributes.
  StringMap attributes_map_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(Attributes);
};

}  // end namespace kmlbase

#endif  // KML_BASE_ATTRIBUTES_H__
