#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "GPSSatelliteData.h"

struct GPSSatelliteReport
{
    static constexpr uint16_t SAT_INFO_MAX_SATELLITES = 128;
    uint64_t timestamp = 0;
    uint16_t count = 0;
    std::optional<GPSConstellation> constellation;
    std::array<GPSSatelliteData, SAT_INFO_MAX_SATELLITES> entries{};
};

struct GPSSatelliteUsageReport
{
    uint64_t timestamp = 0;
    std::optional<int> usedCount;
};
