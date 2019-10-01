// Copyright 2010, Google Inc. All rights reserved.
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

// This file contains the declaration of the CsvSplitter class.

#ifndef KML_BASE_CSV_SPLITTER_H__
#define KML_BASE_CSV_SPLITTER_H__

#include "kml/base/string_util.h"
#include "kml/base/util.h"

namespace kmlbase {

// This class iterates over a string buffer CSV data.  Basic usage:
//  const string csv_data = get-csv-data
//  CsvSplitter csv_splitter(csv_data);
//  StringVector csv_line;
//  while (csv_splitter.SplitCurrentLine(&csv_line)) {
//    ...process the csv_line..
//    csv_line.clear();
//  }
// Note that the methods are virtual to permit overriding in a subclass.
class CsvSplitter {
 public:
  CsvSplitter(const string& csv_data);

  virtual ~CsvSplitter();

  // Find the start of the next line and the end of this line if requested.
  // string::npos is returned if there is no next line.  A line ending is
  // \n or \r or both.
  virtual size_t FindNextLine(size_t* this_end) const;

  // This splits the current line of CSV data into the given StringVector and
  // uses FindNextLine() to advance to the next line if there is one.  True
  // is returned if there is a next line.
  virtual bool SplitCurrentLine(StringVector* cols);

 protected:
  const string csv_data_;
  size_t current_line_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(CsvSplitter);
};

}  // end namespace kmlbase

#endif  // KML_BASE_CSV_SPLITTER_H__
