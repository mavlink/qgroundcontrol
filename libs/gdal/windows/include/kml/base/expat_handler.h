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

// This file declares the C++ ExpatHandler interface to the otherwise C expat.

#ifndef KML_BASE_EXPAT_HANDLER_H__
#define KML_BASE_EXPAT_HANDLER_H__

#include "expat.h" // XML_Char
#include "string_util.h"  // StringVector

namespace kmlbase {
class Attributes;
// This declares the pure virtual ExpatHandler interface.
class ExpatHandler {
public:
  virtual ~ExpatHandler() {}
  virtual void StartElement(const string& name,
                            const StringVector& atts) = 0;
  virtual void EndElement(const string& name) = 0;
  virtual void CharData(const string&) = 0;

  // Namespace handlers with an empty default implementation.
  virtual void StartNamespace(const string& prefix,
                              const string& uri) {}
  virtual void EndNamespace(const string& prefix) {}

  void set_parser(XML_Parser parser) {
    parser_ = parser;
  }
  XML_Parser get_parser() {
    return parser_;
  }

private:
  XML_Parser parser_;
};

const int kBitMask = 0x3f;
const int kByteMask = 0x80;
const int kMask2Bytes = 0xc0;
const int kMask3Bytes = 0xe0;

// Convert an XML_Char buffer to a UTF-8 encoded string.   Even
// if Expat is compiled with Unicode, XML_Char will point to a UTF-16
// encoded character.  It's not known in practice if Expat will actually
// allow surrogate pairs,  but our interface is a pointer in case we find
// an exception to the Unicode's book assertion that no interesting languages
// are represented outside the first 64K Unicode characters.
inline void xmlchar_to_utf8(const XML_Char *input, string* buffer) {
  if (!input || !buffer)
    return;

  const int c = *input;
  // Rely on constant folding and inlining to make this fast when not
  // built with XML_UNICODE; this function should optimize down to an
  // inlined buffer.push_back().
  if (sizeof(XML_Char) == 1 || c < 0x80) {
    buffer->push_back(static_cast<char>(c));
  } else if (c < 0x800) {
    buffer->push_back(kMask2Bytes | c >> 6);
    buffer->push_back(kByteMask | (c & kBitMask));
  } else if (c < 0xd800 || c > 0xdbff) {
    buffer->push_back(kMask3Bytes | c >> 12);
    buffer->push_back(kByteMask | ((c >> 6) & kBitMask));
    buffer->push_back(kByteMask | (c & kBitMask));
  } else {
    // Handle UTF-16 surrogate pairs here.  We 'handle' them by dropping them.
  }
}

inline string xml_char_to_string(const XML_Char *input) {
  string output;

  for (const XML_Char *p = input; input && *p; p++) {
    xmlchar_to_utf8(p, &output);
  }
  return output;
}

inline void xml_char_to_string_vec(const XML_Char **input,
                                   kmlbase::StringVector *ovec) {
  if (!ovec)
    return;
  while (input && *input) {
    ovec->push_back(xml_char_to_string(*input++));
    ovec->push_back(xml_char_to_string(*input++));
  }
}

inline string xml_char_to_string_n(const XML_Char *input, size_t length) {
  string output;
  while (length--) {
    xmlchar_to_utf8(input++, &output);
  }
  return output;
}

}  // end namespace kmlbase

#endif  // KML_BASE_EXPAT_HANDLER_H__
