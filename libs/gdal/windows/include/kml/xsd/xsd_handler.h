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

// This file contains the declaration of the XsdHandler class.

#ifndef KML_XSD_XSD_HANDLER_H__
#define KML_XSD_XSD_HANDLER_H__

#include <stack>
#include "boost/scoped_ptr.hpp"
#include "kml/base/expat_handler.h"
#include "kml/xsd/xsd_type.h"

namespace kmlbase {
class Attributes;
}

namespace kmlxsd {

class XsdFile;

// This ExpatHandler specialization parses an XSD file.  Overall usage is:
//   // Read the XSD file.
//   string xsd_data;
//   File::ReadFileToString("my.xsd", &xsd_data);
//   // Parse it.
//   XsdHandler my_xsd_handler;
//   ExpatParser(xsd_data, &my_xsd_handler);
class XsdHandler : public kmlbase::ExpatHandler {
 public:
  XsdHandler(XsdFile* xsd_file)
    : xsd_file_(xsd_file),
      current_type_(NULL) {
  }
  virtual ~XsdHandler() {}

  // ExpatHandler::StartElement.
  virtual void StartElement(const string& element_name,
                            const kmlbase::StringVector& atts);

  // ExpatHandler::EndElement.
  virtual void EndElement(const string& element_name);

  // ExpatHandler::CharData.  No XSD element has character data.
  virtual void CharData(const string& s) {}

 private:
  XsdFile* xsd_file_;

  // <xs:element> processing.
  void StartXsElement(const kmlbase::Attributes& attributes);

  // <xs:complexType> processing.
  void StartComplexType(const kmlbase::Attributes& attributes);
  void StartExtension(const kmlbase::Attributes& attributes);

  // <xs:simpleType> processing.
  void StartSimpleType(const kmlbase::Attributes& attributes);
  void StartRestriction(const kmlbase::Attributes& attributes);
  void StartEnumeration(const kmlbase::Attributes& attributes);

  void EndType();
  XsdTypePtr current_type_;

  std::stack<string> parse_;
};

}  // end namespace kmlxsd

#endif // KML_XSD_XSD_HANDLER_H__
