// AirMap Platform SDK
// Copyright © 2018 AirMap, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef AIRMAP_DATE_TIME_H_
#define AIRMAP_DATE_TIME_H_

#include <airmap/visibility.h>
#include <cstdint>

#include <memory>
#include <string>

namespace airmap {

class DateTime;
template <typename Tag>
class Duration;

namespace detail {
class Duration;
}  // namespace detail

namespace tag {

struct AIRMAP_EXPORT Hours {};
struct AIRMAP_EXPORT Minutes {};
struct AIRMAP_EXPORT Seconds {};
struct AIRMAP_EXPORT Milliseconds {};
struct AIRMAP_EXPORT Microseconds {};

}  // namespace tag

using Hours        = Duration<tag::Hours>;
using Minutes      = Duration<tag::Minutes>;
using Seconds      = Duration<tag::Seconds>;
using Milliseconds = Duration<tag::Milliseconds>;
using Microseconds = Duration<tag::Microseconds>;

/// Clock marks the reference for time measurements.
class AIRMAP_EXPORT Clock {
 public:
  static DateTime universal_time();
  static DateTime local_time();
};

namespace boost_iso {

AIRMAP_EXPORT DateTime datetime(const std::string &iso_time);
AIRMAP_EXPORT std::string to_iso_string(const DateTime &);

}  // namespace boost_iso

/// DateTime marks a specific point in time, in reference to Clock.
class AIRMAP_EXPORT DateTime {
 public:
  DateTime();
  ~DateTime();
  DateTime(DateTime const &);
  DateTime(DateTime &&);
  DateTime &operator=(const DateTime &);
  DateTime &operator=(DateTime &&);

  DateTime operator+(const detail::Duration &) const;
  Microseconds operator-(const DateTime &) const;
  bool operator==(const DateTime &) const;
  bool operator!=(const DateTime &) const;

  friend std::istream &operator>>(std::istream &, DateTime &);
  friend std::ostream &operator<<(std::ostream &, const DateTime &);

  DateTime date() const;
  Microseconds time_of_day() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl;

  explicit DateTime(std::unique_ptr<Impl> &&);
  friend DateTime Clock::universal_time();
  friend DateTime Clock::local_time();
  friend DateTime boost_iso::datetime(const std::string &iso_time);
  friend std::string boost_iso::to_iso_string(const DateTime &datetime);
};

AIRMAP_EXPORT Hours hours(std::int64_t raw);
AIRMAP_EXPORT Minutes minutes(std::int64_t raw);
AIRMAP_EXPORT Seconds seconds(std::int64_t raw);
AIRMAP_EXPORT Milliseconds milliseconds(std::int64_t raw);
AIRMAP_EXPORT Microseconds microseconds(std::int64_t raw);

namespace detail {

class AIRMAP_EXPORT Duration {
 public:
  Duration();
  ~Duration();
  Duration(Duration const &old);
  Duration &operator=(const Duration &);

  uint64_t total_seconds() const;
  uint64_t total_milliseconds() const;
  uint64_t total_microseconds() const;

  uint64_t hours() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl;

  friend DateTime DateTime::operator+(const detail::Duration &) const;
  friend Microseconds DateTime::operator-(const DateTime &) const;
  friend Microseconds DateTime::time_of_day() const;

  friend Hours airmap::hours(std::int64_t raw);
  friend Minutes airmap::minutes(std::int64_t raw);
  friend Seconds airmap::seconds(std::int64_t raw);
  friend Milliseconds airmap::milliseconds(std::int64_t raw);
  friend Microseconds airmap::microseconds(std::int64_t raw);
};

}  // namespace detail

template <typename Tag>
class AIRMAP_EXPORT Duration : public detail::Duration {};

/// milliseconds_since_epoch returns the milliseconds that elapsed since the UNIX epoch.
AIRMAP_EXPORT uint64_t milliseconds_since_epoch(const DateTime &dt);
/// microseconds_since_epoch returns the microseconds that elapsed since the UNIX epoch.
AIRMAP_EXPORT uint64_t microseconds_since_epoch(const DateTime &dt);
/// from_seconds_since_epoch returns a DateTime.
AIRMAP_EXPORT DateTime from_seconds_since_epoch(const Seconds &s);
/// from_milliseconds_since_epoch returns a DateTime.
AIRMAP_EXPORT DateTime from_milliseconds_since_epoch(const Milliseconds &ms);
/// from_microseconds_since_epoch returns a DateTime.
AIRMAP_EXPORT DateTime from_microseconds_since_epoch(const Microseconds &us);

// moves the datetime forward to the specified hour
AIRMAP_EXPORT DateTime move_to_hour(const DateTime &dt, uint64_t hour);

namespace iso8601 {

/// parse parses a DateTime instance from the string s in iso8601 format.
AIRMAP_EXPORT DateTime parse(const std::string &s);
/// generate returns a string in iso8601 corresponding to 'dt'.
AIRMAP_EXPORT std::string generate(const DateTime &dt);

}  // namespace iso8601

AIRMAP_EXPORT std::ostream &operator<<(std::ostream &to, const detail::Duration &from);

}  // namespace airmap

#endif  // AIRMAP_DATE_TIME_H_
