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

// This file contains the declaration of the KmlFile class.

#ifndef KML_ENGINE_KML_FILE_H__
#define KML_ENGINE_KML_FILE_H__

#include <ostream>
#include <vector>
#include "boost/scoped_ptr.hpp"
#include "kml/base/attributes.h"
#include "kml/base/referent.h"
#include "kml/base/xml_namespaces.h"
#include "kml/base/util.h"
#include "kml/base/xml_file.h"
#include "kml/dom.h"
#include "kml/engine/engine_types.h"
#include "kml/engine/get_link_parents.h"
#include "kml/engine/object_id_parser_observer.h"
#include "kml/engine/shared_style_parser_observer.h"

namespace kmlengine {

class KmlCache;

// The KmlFile class represents the instance of a KML file from a given URL.
// A KmlFile manages an XML id domain and includes an internal map of all
// id'ed Objects, shared styles, and name'ed Schemas and a list of all links.
// KmlFile is a fundamental component of the KML Engine and is central in the
// use of shared style resolution.
class KmlFile : public kmlbase::XmlFile {
 public:
  // This creates a KmlFile from a memory buffer of either KML or KMZ data.
  // In the case of KMZ the KmzFile module's ReadKml() is used to read the
  // KML data from the KMZ archive.  On any parse errors NULL is returned
  // and a human readable error message is saved in the supplied string.
  // The caller is responsible for deleting the KmlFile this creates.
  static KmlFile* CreateFromParse(const string& kml_or_kmz_data,
                                  string *errors);

  // This method is for use with NetCache CacheItem.
  static KmlFile* CreateFromString(const string& kml_or_kmz_data) {
    // Internal KML fetch/parse (styleUrl, etc) errors are quietly ignored.
    return CreateFromParse(kml_or_kmz_data, NULL);
  }

  // This method is for use with KmlCache.  The purpose is to keep set_url()
  // and set_kml_cache() private and at creation-time.
  static KmlFile* CreateFromStringWithUrl(const string& kml_data,
                                          const string& url,
                                          KmlCache* kml_cache);

  // This creates a KmlFile from the given element hierarchy.  This variant of
  // CreateFromImport fails on id duplicates.
  static KmlFile* CreateFromImport(const kmldom::ElementPtr& element);

  // This creates a KmlFile from the given element hierarchy.  This variant of
  // CreateFromImport employs a "last one wins" strategy for id duplicates.
  static KmlFile* CreateFromImportLax(const kmldom::ElementPtr& element);

  // This returns the root element of this KML file.
  const kmldom::ElementPtr get_root() const {
    return kmldom::AsElement(XmlFile::get_root());
  }

  // Deprecated.  Use get_root().
  const kmldom::ElementPtr root() const {
    return get_root();
  }

  // This serializes the KML from the root.  The xmlns() value is added to
  // the root element, the set of namespace prefixes to namespaces is added,
  // and the encoding is set in a prepended XML header:
  //    <?xml version="1.0" encoding="ENCODING">
  //    <kml xmlns="XMLNS" xmlns:PREFIX1="XMLNS1" xmlns:PREFIX2="XMLNS2"...>
  //    ...
  //    </kml>
  bool SerializeToString(string* xml_output) const;

  // This does as SerializeToString() except to an ostream.
  bool SerializeToOstream(std::ostream* xml_output) const;

  // This returns the XML header including the encoding:
  // The default is this: "<?version="1.0" encoding="utf-8"?>
  const string CreateXmlHeader() const;

  // These methods access the XML encoding of the XML file.
  // TODO: set should be create time only.
  void set_encoding(const string& encoding) {
    encoding_ = encoding;
  }
  const string& get_encoding() const {
    return encoding_;
  }

  // This returns the Object Element with the given id.  A NULL Object is
  // returned if no Object with this id exists in the KML file.
  kmldom::ObjectPtr GetObjectById(const string& id) const;

  // This returns the shared StyleSelector Element with the given id.  NULL is
  // returned if no StyleSelector with this id exists as a shared style
  // selector in the KML file.
  kmldom::StyleSelectorPtr GetSharedStyleById(const string& id) const;

  const SharedStyleMap& get_shared_style_map() const {
    return shared_style_map_;
  }

  // This returns the all Elements that may have link children.  See
  // GetLinkParents() for more information.
  const ElementVector& get_link_parent_vector() const {
    return link_parent_vector_;
  }

  // This is the KmlCache which created this KmlFile.  This may be NULL if this
  // KmlFile was not created using CreateFromStringWithUrl().
  KmlCache* get_kml_cache() const {
    return kml_cache_;
  }

  // Duplicate id attributes are illegal and should cause the parse to fail.
  // However, Google Earth never enforced this in its KML ingest and thus the
  // web has a lot of invalid KML. We attempt to parse this by default. A
  // client may use set_strict_parse(true) to override this, which will
  // instruct the ObjectIdParserObserver to fail on duplicate ids.
  void set_strict_parse(bool val) {
    strict_parse_ = val;
  }

 private:
  // Constructor is private.  Use static Create methods.
  KmlFile();

  // This is an internal helper function for the public CreateFromImport*()
  // methods.
  static KmlFile* CreateFromImportInternal(const kmldom::ElementPtr& element,
                                           bool disallow_duplicate_ids);

  // This is an internal method used in the static Create methods.
  bool ParseFromString(const string& kml, string* errors);

  // Only static Create methods can set the KmlCache.
  void set_kml_cache(KmlCache* kml_cache) {
    kml_cache_ = kml_cache;
  }
  // These are helper functions for CreateFromParse().
  bool _CreateFromParse(const string& kml_or_kmz_data,
                        string* errors);
  bool OpenAndParseKmz(const string& kmz_data, string* errors);
  string encoding_;
  // TODO: use XmlElement's id map.
  ObjectIdMap object_id_map_;
  SharedStyleMap shared_style_map_;
  ElementVector link_parent_vector_;
  KmlCache* kml_cache_;
  bool strict_parse_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(KmlFile);
};

typedef boost::intrusive_ptr<KmlFile> KmlFilePtr;

}  // end namespace kmlengine

#endif  // KML_ENGINE_KML_FILE_H__
