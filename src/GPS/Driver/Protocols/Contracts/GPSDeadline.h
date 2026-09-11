#pragma once

#include <algorithm>
#include <cstdint>

struct GPSDeadline
{
    uint64_t untilUs = UINT64_MAX;

    int remainingMilliseconds(uint64_t nowUs) const
    {
        return nowUs >= untilUs ? 0
                                : static_cast<int>(std::min<uint64_t>(
                                      (untilUs - nowUs) / 1000 + ((untilUs - nowUs) % 1000 != 0), INT32_MAX));
    }
};
