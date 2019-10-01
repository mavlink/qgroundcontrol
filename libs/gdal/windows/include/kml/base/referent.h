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

#ifndef KML_BASE_REFERENT_H__
#define KML_BASE_REFERENT_H__

// This file contains the implementation of the Referent class which holds
// the reference counter used by boost::intrusive_ptr.  The Referent class
// is a base class of all KML DOM Elements and also the base TempFile
// class.  Neither the Referent class nor the methods here are part of the
// libkml public API.

namespace kmlbase {

// This class implements the reference count used by boost::intrusive_ptr.
class Referent {
 public:
  // The constructor only constructs the Referent object.  The reference
  // count is incremented if and when the Referent-derived object is assigned
  // to a boost::intrusive_ptr.
  Referent() : ref_count_(0) {}
  virtual ~Referent() {}

  // This method is used by intrusive_ptr_add_ref() to increment the reference
  // count of a given Referent-derived object.
  void add_ref() {
    ++ref_count_;
  }

  // This method is used by intrusive_ptr_release() to decrement the reference
  // count of a given Referent-derived object.
  int release() {
    return --ref_count_;
  }

  // This is for debugging purposes only.
  int get_ref_count() const {
    return ref_count_;
  }

 private:
  int ref_count_;
};

// These declarations are for the implementation of the functions used within
// boost::intrusive_ptr to manage Referent-derived objects..  See referent.cc
// and boost/intrusive_ptr.hpp.
void intrusive_ptr_add_ref(kmlbase::Referent* r);
void intrusive_ptr_release(kmlbase::Referent* r);

} // end namespace kmlbase

#endif  // KML_BASE_REFERENT_H__
