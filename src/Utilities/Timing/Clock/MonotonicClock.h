#pragma once

#include <chrono>
#include <cstdint>

namespace MonotonicClock {
inline uint64_t nowUs()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

/// A missing or future timestamp has no valid age. Both arguments must share a clock domain.
constexpr int64_t ageMilliseconds(uint64_t timestampUs, uint64_t nowUs)
{
    return timestampUs == 0 || timestampUs > nowUs ? -1 : static_cast<int64_t>((nowUs - timestampUs) / 1000);
}
}  // namespace MonotonicClock
