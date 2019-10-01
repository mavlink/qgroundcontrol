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

#ifndef KML_BASE_UTIL_H__
#define KML_BASE_UTIL_H__

// OTB provides stdint.h via msinttypes
//#if (!defined(_MSC_VER)) || (_MSC_VER == 1600)
#include <stdint.h>  // For fixed-size interger typedefs in this file.
//#endif

// A macro to disallow the evil copy constructor and assignment operator.
// Should be used in the private: declarations for a class.
#define LIBKML_DISALLOW_EVIL_CONSTRUCTORS(TypeName) \
  TypeName(const TypeName&);\
  void operator=(const TypeName&)

typedef unsigned int uint;

// OTB provides stdint.h via msinttypes, so we don't need this
#if 0
// MSVC has no header for C99 typedefs. (MSVC 2010 has it)
#ifdef _MSC_VER
#if _MSC_VER < 1600
typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif // _MSC_VER < 1600
#endif  // _MSC_VER
#endif

#include <string>

// A convenience for the internal build system at Google.
#ifndef HAS_GLOBAL_STRING
using std::string;
#endif

#endif  // KML_BASE_UTIL_H__
