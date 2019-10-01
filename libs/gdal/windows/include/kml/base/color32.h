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

// This file contains the declaration of the Color32 class.

#ifndef KML_BASE_COLOR_H__
#define KML_BASE_COLOR_H__

#include "kml/base/string_util.h"
#include "kml/base/util.h"

namespace kmlbase {

class Color32 {
 public:
  explicit Color32()
    : color_abgr_(0xffffffff) {
  }
  explicit Color32(uint32_t abgr)
    : color_abgr_(abgr) {
  }
  explicit Color32(int32_t abgr)
    : color_abgr_(static_cast<uint32_t>(abgr)) {
  }
  Color32(unsigned char a, unsigned char b, unsigned char g, unsigned char r) {
    set_color_abgr((a << 24) | (b << 16) | (g << 8) | r);
  }
  Color32(const string& value) {
    set_color_abgr(value);
  }

  // Red.
  uint32_t get_red() const {
    return color_abgr_ & 0x000000ff;
  }
  void set_red(unsigned char value) {
    color_abgr_ = (color_abgr_ & 0xffffff00) | value;
  }

  // Green.
  uint32_t get_green() const {
    return (color_abgr_ & 0x0000ff00) >> 8;
  }
  void set_green(unsigned char value) {
    color_abgr_ = (color_abgr_ & 0xffff00ff) | value << 8;
  }

  // Blue.
  uint32_t get_blue() const {
    return (color_abgr_ & 0x00ff0000) >> 16;
  }
  void set_blue(unsigned char value) {
    color_abgr_ = (color_abgr_ & 0xff00ffff) | value << 16;
  }

  // Alpha.
  uint32_t get_alpha() const {
    return (color_abgr_ & 0xff000000) >> 24;
  }
  void set_alpha(unsigned char value) {
    color_abgr_ = (color_abgr_ & 0x00ffffff) | value << 24;
  }

  // Returns the color as AABBGGRR.
  uint32_t get_color_abgr() const {
    return color_abgr_;
  }

  // Returns the color as AARRGGBB.
  uint32_t get_color_argb() const {
    return (color_abgr_ & 0xff000000) |
           ((color_abgr_ & 0x00ff0000) >> 16) |
           (color_abgr_ & 0x0000ff00) |
           ((color_abgr_ & 0x000000ff) << 16);
  }

  // Returns a new string in the AABBGGRR format.
  string to_string_abgr() const {
    char out[9];
    b2a_hex(get_red(), out + 6);
    b2a_hex(get_green(), out + 4);
    b2a_hex(get_blue(), out + 2);
    b2a_hex(get_alpha(), out);
    out[8] = 0;
    return out;
  }

  // Returns a new string in the AARRGGBB format.
  string to_string_argb() const {
    char out[9];
    b2a_hex(get_blue(), out + 6);
    b2a_hex(get_green(), out + 4);
    b2a_hex(get_red(), out + 2);
    b2a_hex(get_alpha(), out);
    out[8] = 0;
    return out;
  }

  // Sets the color from an uint32_t of AABBGGRR color.
  void set_color_abgr(uint32_t color_abgr) {
    color_abgr_ = color_abgr;
  }

  // Sets the color from a string of AABBGGRR color.
  void set_color_abgr(const string& color_abgr) {
    uint32_t out = 0;
    // Don't loop over the entire string. We consider only the first
    // 8 characters significant. If the string starts with a "#" character,
    // skip it. (Google Earth supports this usage, despite its not being
    // common practice.)
    size_t offset = 0;
    if (color_abgr.size() > 0 && color_abgr[0] == '#') {
      offset = 1;
    }
    size_t length = color_abgr.size() >= 8 + offset ? 8 : color_abgr.size();
    for(size_t i = offset; i < length + offset; ++i) {
      out = out * 16;
      if (color_abgr[i] >= '0' && color_abgr[i] <= '9') {
        out += color_abgr[i] - '0';
      }
      if (tolower(color_abgr[i]) >= 'a' && tolower(color_abgr[i]) <= 'f') {
        out += tolower(color_abgr[i]) - 'a' + 10;
      }
    }
    set_red(out & 0xff);
    set_green((out >> 8) & 0xff);
    set_blue((out >> 16) & 0xff);
    set_alpha((out >> 24) & 0xff);
  }

  // Sets the color from four unsigned r, g, b, a chars.
  void set_color_abgr(unsigned char a, unsigned char b,
                      unsigned char g, unsigned char r) {
    set_alpha(a);
    set_blue(b);
    set_green(g);
    set_red(r);
  }

  // Sets the color from a uint of AARRGGBB color.
  void set_color_argb(uint32_t color_argb) {
    set_alpha((color_argb >> 24) & 0xff);
    set_red((color_argb >> 16) & 0xff);
    set_green((color_argb >> 8) & 0xff);
    set_blue(color_argb & 0xff);
  }

  // Operator overrides.
  Color32& operator=(uint32_t color_abgr) {
    color_abgr_ = color_abgr;
    return *this;
  }
  Color32& operator=(int32_t color_abgr) {
    color_abgr_ = static_cast<uint32_t>(color_abgr);
    return *this;
  }
  Color32& operator=(const Color32& color) {
    color_abgr_ = color.color_abgr_;
    return *this;
  }
  bool operator!=(const Color32& color) const {
    return !operator == (color);
  }
  bool operator==(const Color32& color) const {
    return color_abgr_ == color.color_abgr_;
  }
  bool operator>(const Color32& color) const {
    return color_abgr_ > color.color_abgr_;
  }
  bool operator<(const Color32& color) const {
    return color_abgr_ < color.color_abgr_;
  }

 private:
  uint32_t color_abgr_;  // Stored in the standard aabbggrr KML format.
};

}  // end namespace kmlbase

#endif // KML_BASE_COLOR_H_
