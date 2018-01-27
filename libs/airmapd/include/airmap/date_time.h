#ifndef AIRMAP_DATE_TIME_H_
#define AIRMAP_DATE_TIME_H_

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace airmap {

/// Clock marks the reference for time measurements.
using Clock = boost::posix_time::microsec_clock;
/// DateTime marks a specific point in time, in reference to Clock.
using DateTime     = boost::posix_time::ptime;
using Hours        = boost::posix_time::hours;
using Minutes      = boost::posix_time::minutes;
using Seconds      = boost::posix_time::seconds;
using Milliseconds = boost::posix_time::milliseconds;
using Microseconds = boost::posix_time::microseconds;

/// milliseconds_since_epoch returns the milliseconds that elapsed since the UNIX epoch.
std::uint64_t milliseconds_since_epoch(const DateTime& dt);
/// microseconds_since_epoch returns the microseconds that elapsed since the UNIX epoch.
std::uint64_t microseconds_since_epoch(const DateTime& dt);
/// from_seconds_since_epoch returns a DateTime.
DateTime from_seconds_since_epoch(const Seconds& s);
/// from_milliseconds_since_epoch returns a DateTime.
DateTime from_milliseconds_since_epoch(const Milliseconds& ms);
/// from_microseconds_since_epoch returns a DateTime.
DateTime from_microseconds_since_epoch(const Microseconds& us);

// moves the datetime forward to the specified hour
DateTime move_to_hour(const DateTime& dt, int hour);

namespace iso8601 {

/// parse parses a DateTime instance from the string s in iso8601 format.
DateTime parse(const std::string& s);
/// generate returns a string in iso8601 corresponding to 'dt'.
std::string generate(const DateTime& dt);

}  // namespace iso8601

}  // namespace airmap

#endif  // AIRMAP_DATE_TIME_H_
