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

// This file contains the declaration of the internal ExpatParser class.
// Direct use of this class in application code is not recommended.  Typical
// applications should use kmlengine::KmlFile for parsing KML (and KMZ) files.
// Note that we explicitly do not support parsing files with XML ENTITY
// declarations. See the unit tests in expat_parser_test.cc for some concrete
// examples.

#ifndef KML_BASE_EXPAT_PARSER_H__
#define KML_BASE_EXPAT_PARSER_H__

#include <map>
#include "expat.h"
#include "kml/base/util.h"

namespace kmlbase {

const char kExpatNsSeparator = '|';

class ExpatHandler;
class ExpatHandlerNs;

typedef std::map<string, ExpatHandler*> ExpatHandlerMap;

class ExpatHandlerSet {
 public:
  ExpatHandlerSet()
    : default_(NULL) {
  }

  void set_handler(const string& xml_namespace,
                  ExpatHandler* expat_handler) {
    if (!default_) {  // TODO: hack
      default_ = expat_handler;
    }
    expat_handler_map_[xml_namespace] = expat_handler;
  }

  // TODO: this is a hack.  only the instance document has a concept of a
  // default namespace (which may have no default namespace at all).
  ExpatHandler* get_default_handler() const {
    return default_;
  }

  // TODO: this is how the parser core really looks up the handler for
  // a given namespace.  This returns NULL if no handler is available for
  // the given namespace.
  ExpatHandler* get_handler(const string& xmlns) const {
    ExpatHandlerMap::const_iterator iter = expat_handler_map_.find(xmlns);
    return iter == expat_handler_map_.end() ? NULL : iter->second;
  }

 private:
  ExpatHandler* default_;
  ExpatHandlerMap expat_handler_map_;
};

// This internal class front-ends expat.  Usage is as follows:
// class SomeXmlLanguageHandler : public kmlbase::ExpatHandler {
//   // See expat_handler.h for methods to implement.
// };
// SomeXmlLanguageHandler some_handler;
// bool status = ExpatParser::ParseString(xml_file_contents, &some_handler,
//                                        &errors, namespace_aware_bool);
// State of parse (if any) is held in the class derived from ExpatHandler.
class ExpatParser {
 public:
  ExpatParser(ExpatHandler* handler, bool namespace_aware);
  ~ExpatParser();

  // Parses a string of XML data in one operation. The xml string must be a
  // complete, well-formed XML document.
  static bool ParseString(const string& xml, ExpatHandler* handler,
                          string* errors, bool namespace_aware);

  // This allocates a buffer for use with ParseInternalBuffer.  The caller is
  // expected to put the next buffer's worth of XML to parse into this buffer.
  void* GetInternalBuffer(size_t size);

  // This sends the data the caller put in the buffer in GetInternalBuffer to
  // the parser.  The size indicates the number of bytes of data in the buffer.
  // If an error string is supplied any error messages are stored there.
  // If this buffer is the final chunk of XML to parse set is_final to true.
  bool ParseInternalBuffer(size_t size, string* errors, bool is_final);

  // Parse a chunk of XML data. The input does not have to be split on element
  // boundaries. The is_final flag indicates to expat if it should consider
  // this buffer the end of the content.
  bool ParseBuffer(const string& input, string* errors,
                   bool is_final);

 private:
  ExpatHandler* expat_handler_;
  XML_Parser parser_;
  // Used by the static ParseString public method.
  bool _ParseString(const string& xml, string* errors);
  void ReportError(XML_Parser parser, string* errors);
};


}  // end namespace kmldom

#endif  // KML_BASE_EXPAT_PARSER_H__
