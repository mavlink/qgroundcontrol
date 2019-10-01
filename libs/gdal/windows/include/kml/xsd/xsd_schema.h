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

// This file contains the declaration of the XsdSchema class.

#ifndef KML_XSD_XSD_SCHEMA_H__
#define KML_XSD_XSD_SCHEMA_H__

#include "boost/intrusive_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "kml/base/attributes.h"
#include "kml/base/referent.h"
#include "kml/base/xmlns.h"

namespace kmlxsd {

// XsdSchema corresponds to <xs:schema ... >
class XsdSchema : public kmlbase::Referent {
public:
  // Create an XsdSchema from the given attributes.  The attributes must
  // include both a targetNamespace="tns" and xmlns:prefix="tns".  All xmlns:'s
  // are processed and saved.
  static XsdSchema* Create(const kmlbase::Attributes& attributes) {
    XsdSchema* xsd_schema = new XsdSchema;
    if (xsd_schema->Parse(attributes)) {
      return xsd_schema;
    }
    delete xsd_schema;
    return NULL;
  }

  // Return the value of the targetNamespace= attribute.
  const string& get_target_namespace() const {
    return target_namespace_;
  }

  // Return the xmlns prefix whose value is the targetNamespace.  For example,
  // if targetNamespace="a:b:c" and xmlns:foo="a:b:c" this returns "foo".
  const string& get_target_namespace_prefix() const {
    return target_namespace_prefix_;
  }

  // If the given namespace qualified name is prefixed with the target
  // namespace prefix then return true and save the local portion to ncname.
  // For example, if ns_name is "kml:LookAt" and the target namespace prefix
  // is "kml" then name is set to "LookAt" and true is returned.
  bool SplitNsName(const string& ns_name, string* name) const {
    size_t prefix_size = target_namespace_prefix_.size();
    if (ns_name.size() > prefix_size + 1 &&
        ns_name.compare(0, prefix_size + 1,
                        target_namespace_prefix_ + ":") == 0) {
      if (name) {
        *name = ns_name.substr(prefix_size + 1);
      }
      return true;
    }
    return false;
  }

 private:
  XsdSchema() {}  // Use static Create().
  // Set state from attributes, returns true if both targetNamespace= and the
  // target namespace prefix were found, false otherwise.
  bool Parse(const kmlbase::Attributes& attributes) {
    attributes.GetString("targetNamespace", &target_namespace_);
    if (target_namespace_.empty()) {
      return false;
    }
    xmlns_.reset(kmlbase::Xmlns::Create(attributes));
    if (!xmlns_.get()) {
      return false;
    }
    // Find the prefix used for the targetNamespace.
    // For example, if xmlns:foo="a:b:c" and targetNamespace="a:b:c" then the
    // prefix we seek is "foo".  A targetNamespace and xmlns:prefix _must_
    // appear in the <schema> for this to be a valid XSD.
    target_namespace_prefix_ = xmlns_->GetKey(target_namespace_);
    return !target_namespace_.empty() && !target_namespace_prefix_.empty();
  }
  boost::scoped_ptr<kmlbase::Xmlns> xmlns_;
  string target_namespace_;
  string target_namespace_prefix_;
};

typedef boost::intrusive_ptr<XsdSchema> XsdSchemaPtr;

}  // end namespace kmlxsd

#endif  // KML_XSD_XSD_SCHEMA_H__
