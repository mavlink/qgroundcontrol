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

// This file contains the TempFile class.

#ifndef KML_BASE_TEMPFILE_H__
#define KML_BASE_TEMPFILE_H__

#include "boost/intrusive_ptr.hpp"
#include "kml/base/file.h"
#include "kml/base/referent.h"

namespace kmlbase {

class TempFile;
typedef boost::intrusive_ptr<kmlbase::TempFile> TempFilePtr;

// A helper class to manage the creation and deletion of temporary files.
// TempFile::CreateTempFile(string) returns a pointer to the class upon
// successful creation of a tempfile. Returns NULL otherwise. The class
// is derived from Referent, which implements the boost::intrusive_ptr
// behavior.
class TempFile : public Referent {
 public:
  static TempFile* CreateTempFile() {
    string tempfile;
    if (!File::CreateNewTempFile(&tempfile)) {
      return NULL;
    }
    return new TempFile(tempfile);
  }
  ~TempFile() {
    if (File::Exists(name_)) {
      File::Delete(name_);
    }
  }
  const string& name() {
    return name_;
  }
 private:
  TempFile(const string& filename) : name_(filename) {
  }
  string name_;
};

}  // end namespace kmlbase

#endif  // KML_BASE_TEMPFILE_H__
