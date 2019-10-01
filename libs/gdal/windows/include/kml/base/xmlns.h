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

// The Xmlns class is deprecated.  Use Attributes.

#ifndef KML_BASE_XMLNS_H__
#define KML_BASE_XMLNS_H__

#include <map>
#include <vector>
#include "boost/scoped_ptr.hpp"
#include "kml/base/attributes.h"

namespace kmlbase {

// This class holds the default namespace and the set of prefix-namespace
// mappings for an XML file.  For example, if these attributes of the root
// element are used with this class's Create...
//  <schema xmlns="http://www.w3.org/2001/XMLSchema"
//          xmlns:kml="http://www.opengis.net/kml/2.2"
//          xmlns:atom="http://www.w3.org/2005/Atom"
//          xmlns:xal="urn:oasis:names:tc:ciq:xsdschema:xAL:2.0"
//          targetNamespace="http://www.opengis.net/kml/2.2"
//          elementFormDefault="qualified"
//          version="2.2.0">
// ...the get_default() will return "http://www.w3.org/2001/XMLSchema", and the
// following prefix-namespaces pairs will be returned by get_namespace:
//          kml="http://www.opengis.net/kml/2.2"
//          atom="http://www.w3.org/2005/Atom"
//          xal="urn:oasis:names:tc:ciq:xsdschema:xAL:2.0"
// If there are no "xmlns*" attribute names in the passed attributes Create()
// returns NULL.
class Xmlns {
 public:
  // The caller owns the created Xmlns object.
  static Xmlns* Create(const kmlbase::Attributes& attributes) {
    Xmlns* xmlns = new Xmlns;
    if (xmlns->Parse(attributes)) {
      return xmlns;
    }
    delete xmlns;
    return NULL;
  }

  // This returns the URI of the default namespace.  The returned string is
  // empty if there is no default namespace.  A default namespace is the value
  // of an "xmlns" attribute (one with no : and prefix), for example the above
  // sample has this default namespace URI: "http://www.w3.org/2001/XMLSchema".
  const string& get_default() const {
    return default_;
  }

  // This returns the URI of the namespace for the given prefix.  The returned
  // string is empty if no such prefix-namespace mapping exists.  In the sample
  // above a prefix of "kml" returns "http://www.opengis.net/kml/2.2".
  const string GetNamespace(const string& prefix) const {
    string name_space;
    if (prefix_map_.get()) {
      prefix_map_->GetValue(prefix, &name_space);
    }
    return name_space;
  }

  // This returns the prefix for the given namespace.  The returned string is
  // empty if no such namespace has a prefix.  In the sample above a namespace
  // of "http://www.opengis.net/kml/2.2" returns "kml".
  const string GetKey(const string& value) const {
    string key;
    if (prefix_map_.get()) {
      prefix_map_->FindKey(value, &key);
    }
    return key;
  }

  // This returns a list of all xmlns prefix names.  For example, from the
  // sample above this returns "kml", "atom", "xal".  Order from the original
  // XML is not preserved (XML attributes in general have no order semantics
  // and must each be unique).
  void GetPrefixes(std::vector<string>* prefix_vector) const {
    if (prefix_map_.get()) {
      prefix_map_->GetAttrNames(prefix_vector);
    }
  }

 private:
  Xmlns() {}
  bool Parse(const kmlbase::Attributes& attributes) {
    // Create a copy so that we can use non-const SplitByPrefix.
    boost::scoped_ptr<Attributes> clone(attributes.Clone());
    prefix_map_.reset(clone->SplitByPrefix("xmlns"));
    attributes.GetValue("xmlns", &default_);
    // Return true if there is a default xmlns or if there are any
    // xmlns:prefx="ns" pairs.
    return !default_.empty() || prefix_map_.get();
  }
  string default_;
  boost::scoped_ptr<Attributes> prefix_map_;
};

}  // end namespace kmlbase

#endif // KML_BASE_XMLNS_H__
