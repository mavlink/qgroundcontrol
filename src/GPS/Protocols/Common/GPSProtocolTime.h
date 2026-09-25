#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <time.h>

// Receiver calendar fields are UTC, irrespective of the host timezone. Normalize
// overflowing fields too: SBF encodes GPS weeks as offsets from January 1980.
static inline time_t gpsTimeToEpoch(tm& utc)
{
    using namespace std::chrono;
    const int64_t monthIndex = static_cast<int64_t>(utc.tm_year) * 12 + utc.tm_mon;
    const int64_t normalizedYear = 1900 + monthIndex / 12 - (monthIndex % 12 < 0 ? 1 : 0);
    if (normalizedYear < int(year::min()) || normalizedYear > int(year::max())) {
        return static_cast<time_t>(-1);
    }
    const unsigned normalizedMonth = static_cast<unsigned>((monthIndex % 12 + 12) % 12 + 1);
    const sys_seconds instant = sys_days(year(static_cast<int>(normalizedYear)) / month(normalizedMonth) / 1) +
                                days(static_cast<int64_t>(utc.tm_mday) - 1) + hours(utc.tm_hour) + minutes(utc.tm_min) +
                                seconds(utc.tm_sec);
    const auto count = instant.time_since_epoch().count();
    if ((count < 0 && (!std::numeric_limits<time_t>::is_signed ||
                       count < static_cast<int64_t>(std::numeric_limits<time_t>::min()))) ||
        (count >= 0 && static_cast<uint64_t>(count) > static_cast<uint64_t>(std::numeric_limits<time_t>::max()))) {
        return static_cast<time_t>(-1);
    }
    const auto date = floor<days>(instant);
    if (date < sys_days(year::min() / January / 1) || date > sys_days(year::max() / December / 31)) {
        return static_cast<time_t>(-1);
    }
    const year_month_day calendar(date);
    const hh_mm_ss time(instant - date);
    utc.tm_year = int(calendar.year()) - 1900;
    utc.tm_mon = static_cast<int>(unsigned(calendar.month())) - 1;
    utc.tm_mday = static_cast<int>(unsigned(calendar.day()));
    utc.tm_hour = static_cast<int>(time.hours().count());
    utc.tm_min = static_cast<int>(time.minutes().count());
    utc.tm_sec = static_cast<int>(time.seconds().count());
    utc.tm_wday = static_cast<int>(weekday(date).c_encoding());
    utc.tm_yday = static_cast<int>((date - sys_days(calendar.year() / January / 1)).count());
    utc.tm_isdst = 0;
    return static_cast<time_t>(count);
}
