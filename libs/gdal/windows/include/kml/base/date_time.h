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

// This file contains the declaration of the DateTime class.

#ifndef KML_BASE_DATE_TIME_H__
#define KML_BASE_DATE_TIME_H__

#include <time.h>
#include "kml/base/util.h"

// TODO: fix this for real.
#ifdef _WIN32
time_t timegm(struct tm* tm);
char* strptime(const char* buf, const char* format, struct tm* tm);
#endif

namespace kmlbase {

class DateTime {
 public:
  // xsd:datetime: 2008-10-03T09:25:42Z
  // TODO: date, gYearMonth, gYear
  static DateTime* Create(const string& str);

  // A convenience utility: Create() + GetTimeT().
  static time_t ToTimeT(const string& str);

  // POSIX time
  time_t GetTimeT() /* const */;

  // XML Schema 3.2.8 time
  string GetXsdTime() const;

  // XML Schema 3.2.9 date
  string GetXsdDate() const;

  // XML Schema 3.2.7 dateTime.
  string GetXsdDateTime() const;

 private:
  DateTime();
  template<int N>
  string DoStrftime(const char* format) const;
  bool ParseXsdDateTime(const string& xsd_date_time);
  struct tm tm_;
};

time_t DateTimeToTimeT(const string& date_time_str);

}  // end namespace kmlbase

#endif  // KML_BASE_DATE_TIME_H__
