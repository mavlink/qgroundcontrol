#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

/// An absolute deadline on the session's GPSClock, which may be virtual. Only GPSDriver's I/O adapter converts it to
/// the QDeadlineTimer the transports wait on.
struct GPSDeadline
{
    uint64_t untilUs = UINT64_MAX;

    /// A negative @a timeout is already due at @a nowUs.
    static GPSDeadline after(uint64_t nowUs, std::chrono::microseconds timeout)
    {
        return {nowUs + static_cast<uint64_t>((std::max) (timeout, std::chrono::microseconds::zero()).count())};
    }

    /// Rounded up to whole milliseconds and capped at INT32_MAX milliseconds.
    std::chrono::milliseconds remaining(uint64_t nowUs) const
    {
        return std::chrono::milliseconds(
            nowUs >= untilUs
                ? 0
                : std::min<uint64_t>((untilUs - nowUs) / 1000 + ((untilUs - nowUs) % 1000 != 0), INT32_MAX));
    }
};
