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

#ifndef KML_BASE_FILE_H__
#define KML_BASE_FILE_H__

#include "kml/base/util.h"

namespace kmlbase {

class File {
 public:

  // Reads a file into a string. Returns false if given a bad file descriptor
  // or if the file could not be opened. output is unmodified on failure.
  static bool ReadFileToString(const string& filename,
                               string* output);

  // Writes a string into a file. Returns false if the target file could
  // not be created and opened for writing.
  static bool WriteStringToFile(const string& data,
                                const string& filename);

  // Returns true if the file exists.
  static bool Exists(const string& full_path);

  // Deletes a file. If the file does not exist, returns false. Returns true
  // if the file was deleted.
  static bool Delete(const string& filepath);

  // Creates a unique file in the system temporary directory. Returns the
  // full path of the new file in 'path'.
  // Returns true if the function succeeds. 'path' is unmodified on failure.
  static bool CreateNewTempFile(string* full_filepath);

  // Join two file paths. If the first does not end in the platform-specific
  // path separator, it is appended before the second string is joined. Returns
  // the joined string. If either of the strings is empty, the other string is
  // returned unmodified. This should NOT be used with URL paths, which are
  // not platform-specific.
  static string JoinPaths(const string& p1, const string& p2);

  // Splits a path to a filename into its base directory and filename
  // components. E.g. /tom/dick/harry.txt is "/tom/dick" and "harry.txt".
  // Either of the string pointers may be NULL.
  static void SplitFilePath(const string& filepath,
                            string* base_directory,
                            string* filename);
};

}  // end namespace kmlbase

#endif  // KML_BASE_FILE_H__
