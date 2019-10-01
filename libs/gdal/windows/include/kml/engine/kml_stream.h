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

// This file contains the declaration of the KmlStream class.

#ifndef KML_ENGINE_KML_STREAM_H__
#define KML_ENGINE_KML_STREAM_H__

#include <istream>
#include "kml/dom.h"
#include "kml/base/util.h"
#include "kml/base/xml_file.h"

namespace kmldom {
class ParserObserver;
}

namespace kmlengine {

// This class is for processing a large KML file in a streamed fashion.
// Unlike the KmlFile class this does NOT build a map of object and shared
// style ids.  This does still build a full KML DOM (unless the user-supplied
// ParserObserver inhibits parenting of certain children).
class KmlStream : public kmlbase::XmlFile {
 public:
  // Create a KmlFile from KML/KMZ in the given C++ istream.  The entire
  // input is consumed.  On any parse or I/O failure NULL is returned and an
  // error message is set to the given error string if one is supplied.
  // If a ParserObserver is supplied it is used during parse.
  static KmlStream* ParseFromIstream(std::istream* input, string* errors,
                                     kmldom::ParserObserver* observer);

  // This returns the root element of this KML stream.
  const kmldom::ElementPtr get_root() const {
    return kmldom::AsElement(XmlFile::get_root());
  }

 private:
  // Constructor is private.  Use static creation methods.
  KmlStream() {}

  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(KmlStream);
};

}  // end namespace kmlengine

#endif  // KML_ENGINE_KML_STREAM_H__
