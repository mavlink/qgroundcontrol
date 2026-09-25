#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

#include <QtCore/QDeadlineTimer>

struct GPSDeadline
{
    uint64_t untilUs = UINT64_MAX;

    int remainingMilliseconds(uint64_t nowUs) const
    {
        return nowUs >= untilUs ? 0
                                : static_cast<int>(std::min<uint64_t>(
                                      (untilUs - nowUs) / 1000 + ((untilUs - nowUs) % 1000 != 0), INT32_MAX));
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
