#pragma once

#include <cstdint>
#include <optional>

#include <QtCore/QMetaType>

/// A used-satellite count, independent of the availability of individual satellite observations.
struct GPSSatelliteUsageReport
{
    uint64_t timestampUs = 0;
    std::optional<int> usedCount = std::nullopt;
};
Q_DECLARE_METATYPE(GPSSatelliteUsageReport)
