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

// This file contains the declaration of the XsdParser class.

#ifndef KML_XSD_XST_PARSER_H__
#define KML_XSD_XST_PARSER_H__

#include <vector>
#include "kml/base/util.h"

namespace kmlxsd {

class XsdFile;

// An ".xst" file is a simple textual representation of a ".xsd".  At present
// this is limited to defining aliases to names used in the XSD.
// TODO: implement element, simpleType (enums), complexType (child elements).
// Overall usage is as follows:
//   XsdFile* xsd_file = XsdFile::CreateFromParse(...);
//   XstParser xst_parser(&xsd_file);
//   string xst_data = read_xst_file();
//   xst_parser.ParseXst(&xst_data);
class XstParser {
 public:
  // Construct the XstParser with an XsdFile to write into.
  XstParser(XsdFile* xsd_file)
    : xsd_file_(xsd_file) {
  }

  // Parse the contents of the XST data into the XsdFile.
  void ParseXst(const string& xst_data);

  // Parse the "alias" line into the XsdFile.  An alias line takes this form:
  //   "alias real_name alias_name"
  void ParseXstAlias(const std::vector<string>& alias_line);

 private:
  XsdFile* xsd_file_;
};

} //  end namespace kmlxsd

#endif  // KML_XSD_XST_PARSER_H__
