#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

/// Timestamps from one monotonic clock domain. A zero (or negative) timestamp means "never", and a timestamp later
/// than now has no age.
namespace MonotonicClock {
inline uint64_t nowUs()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

/// Age of @a timestamp at @a now, both in the same unit; empty for a missing or future timestamp.
template <typename Rep, typename Period>
constexpr std::optional<std::chrono::duration<Rep, Period>> age(std::chrono::duration<Rep, Period> timestamp,
                                                                std::chrono::duration<Rep, Period> now)
{
    if (timestamp <= timestamp.zero() || timestamp > now) {
        return std::nullopt;
    }
    return now - timestamp;
}

constexpr std::optional<std::chrono::microseconds> age(uint64_t timestampUs, uint64_t nowUs)
{
    return age(std::chrono::microseconds(timestampUs), std::chrono::microseconds(nowUs));
}

/// Time left before a receipt reaches @a lifetime; zero for a missing, future or expired one.
constexpr std::chrono::microseconds remaining(uint64_t timestampUs, uint64_t nowUs, std::chrono::microseconds lifetime)
{
    const auto receiptAge = age(timestampUs, nowUs);
    if (!receiptAge || *receiptAge >= lifetime) {
        return std::chrono::microseconds::zero();
    }
    return lifetime - *receiptAge;
}

/// Whether a receipt is younger than @a lifetime.
constexpr bool fresh(uint64_t timestampUs, uint64_t nowUs, std::chrono::microseconds lifetime)
{
    return remaining(timestampUs, nowUs, lifetime) > std::chrono::microseconds::zero();
}

/// Whether a receipt is at most @a maximumAge old, its limit included.
constexpr bool withinAge(uint64_t timestampUs, uint64_t nowUs, std::chrono::microseconds maximumAge)
{
    const auto receiptAge = age(timestampUs, nowUs);
    return receiptAge && *receiptAge <= maximumAge;
}
}  // namespace MonotonicClock
