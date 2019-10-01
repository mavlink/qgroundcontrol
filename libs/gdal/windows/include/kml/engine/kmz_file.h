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

// This file contains the declaration of the KmzFile class.

#ifndef KML_ENGINE_KMZ_FILE_H__
#define KML_ENGINE_KMZ_FILE_H__

#include <vector>
#include "boost/intrusive_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "kml/base/referent.h"
#include "kml/base/util.h"
#include "kml/engine/kml_file.h"

// ZipFile hides the implementation details of the underlying zip library from
// this interface.
namespace kmlbase {
class ZipFile;
}

namespace kmlengine {

// The Kmz class represents an instance of a KMZ file. It contains methods
// for reading and writing KMZ files. By default, there is an upper limit of
// 2 GB on uncompressed file sizes. If you need to lower this limit, use
// the set_max_uncompressed_size method.
class KmzFile : public kmlbase::Referent {
 public:
  ~KmzFile();

  // Open a KMZ file from a file path. Returns a pointer to a KmzFile object
  // if the file could be opened and read, and the data was recognizably KMZ.
  // Otherwise returns NULL.
  static KmzFile* OpenFromFile(const char* kmz_filepath);

  // Open a KMZ file from a string. Returns a pointer to a KmzFile object if a
  // temporary file could be created, the data was recognizably KMZ. Otherwise
  // returns NULL.
  static KmzFile* OpenFromString(const string& kmz_data);

  static KmzFile* CreateFromString(const string& kmz_data) {
    return OpenFromString(kmz_data);
  }

  // Sets the upper limit for the largest uncompressed file size (in bytes)
  // for the underlying Zip implementation to handle. By default it is 2 GB.
  // If this is exceeded, any attempt to read the archived file will return
  // false.
  void set_max_uncompressed_file_size(unsigned int i);

  // Returns the maximum uncompressed file size that the underlying Zip
  // implementation will handle in bytes.
  unsigned int get_max_uncompressed_file_size();

  // Checks to see if kmz_data looks like a PK ZIP file.
  static bool IsKmz(const string& kmz_data);

  // Read the default KML file from a KMZ archive. This is defined as the first
  // entry in the ZIP table of contents that ends in ".kml". Note that it may
  // NOT be at the root level of the archive. The ZIP archives table of
  // contents is exactly the order in which the source files were added to the
  // archive. Returns false if no KML file. The output string is not cleared
  // before being written to.
  bool ReadKml(string* output) const;

  // This does the same as ReadKml() and in addition returns the path of the
  // KML file within the KMZ archive if a non-NULL kml_path is supplied.
  // NOTE: While it is considered a best practice to have The KML file of
  // a KMZ archive be "doc.kml" this is not always the case.
  bool ReadKmlAndGetPath(string* output, string* kml_path) const;

  // Read a specific file from a KMZ archive. Returns false if subfile was not
  // found, or if subfile could not be read. Note: subfile must be a full path
  // from the archive root. Relative references of "../../foo" are not handled.
  // The output string is not cleared before being written to.
  bool ReadFile(const char* subfile, string* output) const;

  // Fills a vector of strings of the files contained in the opened KMZ archive.
  // The vector is not cleared, only appended to. The string is the full path
  // name of the KML file from the archive root, with '/' as the separator.
  // Returns false upon error.
  bool List(std::vector<string>* subfiles);

  // Saves the raw bytes of the in-memory KMZ file.
  bool SaveToString(string* kmz_bytes);

  // These are for the creation of KMZ files:

  // Creates an empty KmzFile at kmz_filepath on which AddFile may be called.
  // Returns NULL if the file could not be created for writing.
  static KmzFile* Create(const char* kmz_filepath);

  // Writes data to path_in_kmz. The path must be relative to the root of the
  // archive. e.g. AddFile(data, "somedir/file.png"). If not, false is returned.
  // False is also returned on any interal zipfile error.
  bool AddFile(const string& data, const string& path_in_kmz);

  // Adds a StringVector of hrefs to the KMZ file, resolved against a base
  // URL. The base URL is usually from kmz_file->get_url() and the hrefs
  // are most easily generated from GetRelativeLinks. All paths are normalized
  // prior to writing.
  // Returns the number of errors encountered during processing.
  // Errors may result from failure to normalize an href, an href that points
  // above the base url, or failure to read the resolved file prior to writing.
  // Duplicate entries are ignored and not considered errors.
  size_t AddFileList(const string& base_url,
                     const kmlbase::StringVector& file_paths);

  // Creates a KMZ file from a string of KML data. Returns true if
  // kmz_filepath could be successfully created and written.
  // TODO: Permit adding resources (images, models, etc.) to the KMZ archive.
  static bool WriteKmz(const char* kmz_filepath, const string& kml);

  // Creates a KMZ file at kmz_filepath from a string of KML. Any local
  // references in the file are written to the KMZ as archived resources
  // according to the rules explained in CreateFromElement.
  // TODO: handle <Model> references.
  // TODO: handle references in <description>.
  static bool CreateFromKmlFilepath(const string& kml_filepath,
                                    const string& kmz_filepath);

  // Creates a KMZ file at kmz_filepath from an ElementPtr and a base url. Any
  // local references in the file are written to the KMZ as archived resources
  // if and only if the resource URI is relative to and below the base_url.
  // i.e. <href>/etc/passwd</href> is not valid because it is absolute, and
  // from a base url of "/home/libkml/" <href>../../etc/passwd</href> is
  // invalid because it does not point below /home/libkml.
  // TODO: handle <Model> references.
  // TODO: handle references in <description>.
  static bool CreateFromElement(const kmldom::ElementPtr& element,
                                const string& base_url,
                                const string& kmz_filepath);

  // Creates a KMZ file at kmz_filepath from a KmlFile. Any local
  // references in the file are written to the KMZ as archived resources
  // according to the rules laid out above for CreateFromElement.
  // The KmlFile _must_ have been created with its base URL set to the
  // local path where the KML file can be found, i.e.
  // KmlPtr kml_file = KmlFile::CreateFromStringWithUrl(...).
  // TODO: handle <Model> references.
  // TODO: handle references in <description>.
  static bool CreateFromKmlFile(const KmlFilePtr& kml_file,
                                const string& kmz_filepath);

 private:
  // Class can only be created from static methods.
  KmzFile(kmlbase::ZipFile* zip_file);
  boost::scoped_ptr<kmlbase::ZipFile> zip_file_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(KmzFile);
};

typedef boost::intrusive_ptr<KmzFile> KmzFilePtr;

}  // end namespace kmlengine

#endif  // KML_ENGINE_KMZ_FILE_H__
