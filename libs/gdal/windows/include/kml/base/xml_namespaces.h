// Copyright 2009, Google Inc. All rights reserved.
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

#ifndef KML_BASE_XML_NAMESPACES_H__
#define KML_BASE_XML_NAMESPACES_H__

#include "kml/base/util.h"

namespace kmlbase {

// These identify KML-related XML namespaces.  Most are not used by KML or
// by libkml and are here only as a convenience when parsing other XML
// langauges that may include KML.  Many are specific to the Google Data APIs
// as indicated.
typedef enum {
  XMLNS_NONE,
  XMLNS_APP,
  XMLNS_ATOM,
  XMLNS_BATCH,  // Google Data
  XMLNS_DOCS,  // Google Data
  XMLNS_EXIF,
  XMLNS_GD,  // Google Data
  XMLNS_GEORSS,
  XMLNS_GML,
  XMLNS_GPHOTO,  // Google Data
  XMLNS_GPX,
  XMLNS_GX22,
  XMLNS_GS,  // Google Data
  XMLNS_KML22,
  XMLNS_MEDIA,
  XMLNS_OPENSEARCH,
  XMLNS_XAL,
  XMLNS_XML,
  XMLNS_XSD,
  XMLNS_XSI
} XmlnsId;

// This data structure associates the libkml-internal xmlns id with the
// namespace's typical prefix and the full namespace name.  All ids, prefixes,
// and namespaces must be unique.
struct XmlNamespace {
  XmlnsId xmlns_id_;
  const char* prefix_;
  const char* xml_namespace_;
};

// This function returns a libkml-specific prefix and XML namespace for the
// given id.  This exists such that elements within libkml need only hold
// a small identifier instead of a full namespace string.  Either prefix or
// xml_namespace pointers can be NULL.  The return value indicates whether
// the given id is known to libkml.
bool FindXmlNamespaceAndPrefix(XmlnsId xmlns_id, string* prefix,
                               string* xml_namespace);

}  // end namespace kmlbase

#endif  // KML_BASE_XML_NAMESPACES_H__
