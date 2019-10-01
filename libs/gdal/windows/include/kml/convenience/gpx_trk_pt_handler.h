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

// This file contains the declaration of the GpxTrkPtHandler class.

#ifndef KML_CONVENIENCE_GPX_TRK_PT_HANDLER_H__
#define KML_CONVENIENCE_GPX_TRK_PT_HANDLER_H__

#include <cstring>  // strcmp
#include "boost/scoped_ptr.hpp"
#include "kml/base/attributes.h"
#include "kml/base/expat_handler.h"
#include "kml/base/vec3.h"

namespace kmlconvenience {

// Find all <trkpt>'s in the GPX file.
// For example:
// <trkpt lat="-33.911973070" lon="18.422974152">
//   <ele>4.943848</ele>
//   <time>2008-10-11T14:55:41Z</time>
// </trkpt>
// Each <trkpt> results in a call to HandlePoint().
// Overall usage: Derive a class from GpxTrkPtHandler with an implementation of
// HandlePoint().
class GpxTrkPtHandler : public kmlbase::ExpatHandler {
 public:

  // ExpatHandler::StartElement()
  virtual void StartElement(const string& name,
                            const std::vector <string>& atts) {
    if (name.compare("trkpt") == 0) {
      // <trkpt lat="-33.911973070" lon="18.422974152">
      // If both lat and lon exist and are sane doubles create a Vec3 for
      // the point.
      boost::scoped_ptr<kmlbase::Attributes> attributes(
          kmlbase::Attributes::Create(atts));
      if (attributes.get()) {
        double latitude;
        double longitude;
        if (attributes->GetDouble("lat", &latitude) &&
            attributes->GetDouble("lon", &longitude)) {
          vec3_.reset(new kmlbase::Vec3(longitude, latitude));
        }
      }
      time_.clear();
    } else if (name.compare("time") == 0  ||
               name.compare("ele") == 0) {
      // <time>2008-10-11T14:55:41Z</time>
      // <ele>4.943848</ele>
      gather_char_data_ = true;
      char_data_.clear();
    }
  }

  // ExpatHandler::EndElement()
  virtual void EndElement(const string& name) {
    if (name.compare("trkpt") == 0) {
      // </trkpt>
      // If a Vec3 was created for this element call the handler.
      if (vec3_.get()) {
        HandlePoint(*vec3_, time_);
      }
    } else if (name.compare("time") == 0) {
      // <time>2008-10-11T14:55:41Z</time>
      time_ = char_data_;
    } else if (name.compare("ele") == 0) {
      // <ele>4.943848</ele>
      if (vec3_.get()) {
        vec3_->set_altitude(strtod(char_data_.c_str(), NULL));
      }
    }
  }

  // ExpatHandler::CharData()
  virtual void CharData(const string& str) {
    if (gather_char_data_) {
      char_data_.append(str);
    }
  }

  // This is called for each <trkpt>.  This default implemenation does nothing.
  virtual void HandlePoint(const kmlbase::Vec3& where,
                           const string& when) {
  };

 private:
  // A fresh Vec3 is created for each <trkpt>.
  boost::scoped_ptr<kmlbase::Vec3> vec3_;
  string time_;
  bool gather_char_data_;
  string char_data_;
};

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_GPX_TRK_PT_HANDLER_H__
