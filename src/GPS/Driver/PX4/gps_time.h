/****************************************************************************
 *
 *   Copyright (c) 2012-2018 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#pragma once

#include <time.h>
#include <cstdint>
#include <limits>

#ifndef NO_MKTIME
// Receiver calendar fields are UTC, irrespective of the host timezone. Normalize
// overflowing fields too: SBF encodes GPS weeks as offsets from January 1980.
static inline time_t gpsTimeToEpoch(tm &utc)
{
#ifdef _WIN32
	return _mkgmtime(&utc);
#elif defined(__NEWLIB__) || defined(GPS_NO_TIMEGM)
	// Newlib has gmtime_r(), but does not provide timegm(). GPS_NO_TIMEGM
	// also selects this path for other platforms with the same libc contract.
	const auto floorDivide = [](int64_t value, int64_t divisor) {
		return value / divisor - (value % divisor < 0 ? 1 : 0);
	};
	const auto daysBeforeYear = [&floorDivide](int64_t year) {
		const int64_t previous = year - 1;
		return previous * 365 + floorDivide(previous, 4)
		       - floorDivide(previous, 100) + floorDivide(previous, 400);
	};
	const int64_t extra_years = floorDivide(utc.tm_mon, 12);
	const int64_t year = static_cast<int64_t>(utc.tm_year) + 1900 + extra_years;
	const int month = static_cast<int>(utc.tm_mon - extra_years * 12);
	static constexpr int days_before_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
	const int64_t days = daysBeforeYear(year) - daysBeforeYear(1970)
			     + days_before_month[month] + (month > 1 && leap ? 1 : 0)
			     + static_cast<int64_t>(utc.tm_mday) - 1;
	const int64_t seconds = days * 86400 + static_cast<int64_t>(utc.tm_hour) * 3600
			       + static_cast<int64_t>(utc.tm_min) * 60 + utc.tm_sec;

	if (seconds < 0) {
		if (!std::numeric_limits<time_t>::is_signed
		    || seconds < static_cast<int64_t>(std::numeric_limits<time_t>::min())) {
			return static_cast<time_t>(-1);
		}

	} else if (static_cast<uint64_t>(seconds) > static_cast<uint64_t>(std::numeric_limits<time_t>::max())) {
		return static_cast<time_t>(-1);
	}

	const time_t epoch = static_cast<time_t>(seconds);
	tm normalized{};

	if (gmtime_r(&epoch, &normalized) == nullptr) {
		return static_cast<time_t>(-1);
	}

	utc = normalized;
	return epoch;
#else
	return timegm(&utc);
#endif
}
#endif
