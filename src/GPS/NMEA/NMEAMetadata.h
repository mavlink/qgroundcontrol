#pragma once

#include <cstdint>
#include <optional>

namespace NMEA {
inline bool freshAt(uint64_t receivedAtUs, uint64_t nowUs, uint64_t maximumAgeUs)
{
    return nowUs >= receivedAtUs && nowUs - receivedAtUs <= maximumAgeUs;
}

struct EpochReceipt
{
    std::optional<int> time;
    uint64_t receivedAtUs = 0;

    bool matches(const EpochReceipt& position, uint64_t maximumAgeUs) const
    {
        return time && time == position.time && freshAt(receivedAtUs, position.receivedAtUs, maximumAgeUs);
    }
};

/// Resolve a fresh time-of-day against the nearest day in a receiver's UTC reference.
inline uint64_t utcAtTimeOfDay(uint64_t referenceUtcUs, uint64_t referenceReceiptUs, std::optional<int> timeMs,
                               uint64_t nowUs, uint64_t maximumAgeUs)
{
    if (!referenceUtcUs || !timeMs || !freshAt(referenceReceiptUs, nowUs, maximumAgeUs)) {
        return 0;
    }
    constexpr int64_t DAY_US = 86400000000;
    const auto reference = static_cast<int64_t>(referenceUtcUs);
    int64_t result = reference - reference % DAY_US + int64_t(*timeMs) * 1000;
    if (result - reference > DAY_US / 2) {
        result -= DAY_US;
    } else if (reference - result > DAY_US / 2) {
        result += DAY_US;
    }
    return result > 0 ? static_cast<uint64_t>(result) : 0;
}
}  // namespace NMEA
