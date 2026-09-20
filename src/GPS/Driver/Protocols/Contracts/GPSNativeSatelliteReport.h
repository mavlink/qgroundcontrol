#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "GPSNativeSatelliteData.h"
#include "GPSSatelliteUsageReport.h"

struct GPSNativeSatelliteReport
{
    static constexpr uint16_t SAT_INFO_MAX_SATELLITES = 128;

    struct Usage
    {
        uint64_t timestamp = 0;
        uint16_t count = 0;
        std::array<uint16_t, SAT_INFO_MAX_SATELLITES> ids{};
    };

    // A zero receipt carries no new view; usage can still update independently.
    uint64_t timestamp = 0;
    uint16_t count = 0;
    std::optional<GPSConstellation> constellation;
    std::array<GPSNativeSatelliteData, SAT_INFO_MAX_SATELLITES> entries{};
    std::optional<Usage> usage;
};
