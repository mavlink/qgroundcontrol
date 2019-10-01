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

// This file contains the declarations of the Version API.

#ifndef KML_BASE_VERSION_H__
#define KML_BASE_VERSION_H__

#include "kml/base/util.h"

namespace kmlbase {

// Include this file and use these #define's in your code such that your
// code can call the methods of the Version API to verify compatibility between
// your code and libkml.  It is the intention that these values match those
// used in the configure.ac file's AC_INIT.
#define LIBKML_MAJOR_VERSION 1
#define LIBKML_MINOR_VERSION 3
#define LIBKML_MICRO_VERSION 0

// This API provides the version info this library was compiled with.
class Version {
 public:
  // This returns the major.minor.micro in string form.
  static string GetString();

  // This is an advisory method which provides the given libkml instance the
  // oportunity to guess at its compatibility with the given version info.
  // In general this will return true for a match on major and any minor
  // greater than or equal to the compiled-in minor.
  static bool IsCompat(int major, int minor, int micro);

  // This returns the value LIBKML_MAJOR_VERSION this libkml was compiled with.
  static int get_major();

  // This returns the value LIBKML_MINOR_VERSION this libkml was compiled with.
  static int get_minor();

  // This returns the value LIBKML_MICRO_VERSION this libkml was compiled with.
  static int get_micro();
};

}  // end namespace kmlbase

#endif  // KML_BASE_VERSION_H__

