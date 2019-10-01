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
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR FILEIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE FILEIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file contains the declaration of the ZipFile class.

#ifndef KML_BASE_ZIP_FILE_H__
#define KML_BASE_ZIP_FILE_H__

#include "boost/scoped_ptr.hpp"
#include "kml/base/string_util.h"
#include "kml/base/util.h"

namespace kmlbase {

// Forward-declare the internal MinizipFile class that hides our current use
// of minizip.
class MinizipFile;

// This class represents a ZIP file. Obviously the intent within this project
// is for use with KMZ files, but this class has no particular KML or KMZ
// specifics.
class ZipFile {
 public:
  // Open a ZIP file in-memory suitable for reading. Will return NULL on any
  // internal error.
  static ZipFile* OpenFromString(const string& zip_data);

  // Open a ZIP file at file_path suitable for reading. Will return NULL on any
  // internal error.
  static ZipFile* OpenFromFile(const char* file_path);

  // Create a ZIP file suitable for writing. Will return NULL on any internal
  // error or a failure to create a file at file_path.
  static ZipFile* Create(const char* file_path);

  ~ZipFile();

  // The default maximum uncompressed file size we permit the underlying
  // zip reader to handle is 2 GB by default.
  void set_max_uncompressed_file_size(unsigned int i) {
    max_uncompressed_file_size_ = i;
  }
  unsigned int get_max_uncompressed_file_size() {
    return max_uncompressed_file_size_;
  }

  // Returns true if zip_data looks like a PK ZIP archive. This is the only
  // supported ZIP variant.
  static bool IsZipData(const string& zip_data);

  // Finds the first file in the ZIP file that ends with the given file
  // extension and writes the entire path into path_in_zip. Returns false
  // if no file with the given extension exists in the archive or if
  // path_in_zip is NULL.
  bool FindFirstOf(const string& file_extension,
                   string* path_in_zip) const;

  // Returns the table of contents for the ZIP file. The StringVector
  // is not cleared before writing. Returns false if the pointer is invalid.
  // The list is simply an enumeration of the files with their full pathnames.
  // With respect to ZIP files, there is no concrete concept of a traditional
  // directory; thus any name with a path separator ("/", etc) has no special
  // treatment. It is the client's responsibility to supply such handling.
  bool GetToc(StringVector* subfiles) const;

  // Is the requested path in the ZIP file's table of contents?
  bool IsInToc(const string& path_in_zip) const;

  // Returns the contents of path_in_zip in the ZIP file. Returns true
  // if path_in_zip exists in the ZIP file. If output is a valid pointer
  // the data of path_in_zip are read into it.
  bool GetEntry(const string& path_in_zip, string* output) const;

  // Returns the raw bytes of this ZipFile.
  const string& get_data() const { return data_; }

  // Writes data to path_in_zip. The path must be relative to the root of the
  // archive. e.g. AddEntry(data, "somedir/file.png"). Specifically, paths that
  // start with a '/' or '..' will be rejected and false is returned. False is
  // also returned on any internal ZIP file error. The ZipFile instance must
  // have been created with ZipFile::Create. If it wasn't false is returned.
  // Note that a second call to AddEntry with new data to the same path is
  // essentially a NOP. True will be returned, but the data is unchanged.
  bool AddEntry(const string& data, const string& path_in_zip);

 private:
  // The constructor used to open a ZIP file in-memory, suitable for reading.
  ZipFile(const string& data);
  // The constructor used in creation of a ZIP file suitable for writing.
  ZipFile(MinizipFile* minizip_file);
  boost::scoped_ptr<MinizipFile> minizip_file_;
  string data_;
  StringVector zipfile_toc_;
  unsigned long max_uncompressed_file_size_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(ZipFile);
};

}  // end namespace kmlbase

#endif  // KML_BASE_ZIP_FILE_H__
