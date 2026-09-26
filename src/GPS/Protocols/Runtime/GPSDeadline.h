#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

#include <QtCore/QDeadlineTimer>

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

    /// Live I/O only: untilUs must use the steady-clock epoch, not an injected test clock.
    QDeadlineTimer toQDeadlineTimer() const
    {
        if (untilUs == UINT64_MAX) {
            return QDeadlineTimer(QDeadlineTimer::Forever, Qt::PreciseTimer);
        }
        using DeadlineTime = std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>;
        return QDeadlineTimer(DeadlineTime(std::chrono::microseconds(untilUs)), Qt::PreciseTimer);
    }
};
