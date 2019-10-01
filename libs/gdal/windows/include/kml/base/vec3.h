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

// This file contains the Vec3 class.

#ifndef KML_BASE_VEC3_H__
#define KML_BASE_VEC3_H__

namespace kmlbase {

// A Vec3 represents a 2d or 3d point.  A Vec3 always has at least longitude
// and latitude.  Altitude defaults to 0 and has_altitude() returns false if
// altitude was not set.
class Vec3 {
 public:
  // Create an empty Vec3.
  Vec3() {
    vec_[0] = vec_[1] = 0.0;
    clear_altitude();
  }

  // Create a 2d Vec3.
  Vec3(double longitude, double latitude) {
    vec_[0] = longitude;
    vec_[1] = latitude;
    clear_altitude();
  }

  // Create a 2d Vec3.
  Vec3(double longitude, double latitude, double altitude) {
    vec_[0] = longitude;
    vec_[1] = latitude;
    set_altitude(altitude);
  }

  void set(int i, double val) {
    if (i == 2) {
      set_altitude(val);
    } else {
      vec_[i] = val;
    }
  }

  double get_longitude() const {
    return vec_[0];
  }
  double get_latitude() const {
    return vec_[1];
  }

  bool has_altitude() const {
    return has_altitude_;
  }
  double get_altitude() const {
    return vec_[2];
  }
  void set_altitude(double altitude) {
    vec_[2] = altitude;
    has_altitude_ = true;
  }
  void clear_altitude() {
    vec_[2] = 0;
    has_altitude_ = false;
  }

  // This class does double-duty as the representation of both gx:coord and
  // gx:angles. In the future we might need to split these out as separate
  // classes. For instance, the initial specifiction of these new elements
  // is unclear on how too few or too many tuples should be handled. For now
  // we treat them exactly as the old-style coordinates.
  double get_heading() const { return get_longitude(); }
  double get_pitch() const { return get_latitude(); }
  double get_roll() const { return get_altitude(); }

  // Operator overrides.
  bool operator==(const Vec3& vec3) const {
    return vec_[0] == vec3.get_longitude() &&
           vec_[1] == vec3.get_latitude() &&
           vec_[2] == vec3.get_altitude();
  }

 private:
  double vec_[3];
  bool has_altitude_;
};

}  // namespace kmlbase

#endif  // KML_BASE_VEC3_H__
