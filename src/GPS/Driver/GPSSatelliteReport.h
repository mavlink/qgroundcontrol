#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "GPSConstellation.h"

struct GPSProtocolSatellite
{
    uint16_t id = 0;
    uint16_t prn = 0;
    GPSConstellation constellation = GPSConstellation::Unknown;
    std::optional<bool> used;
    std::optional<double> elevation;
    std::optional<double> azimuth;
    std::optional<int> signal;
};

struct GPSSatelliteReport
{
    static constexpr uint16_t SAT_INFO_MAX_SATELLITES = 128;
    uint64_t timestamp = 0;
    uint16_t count = 0;
    std::array<GPSProtocolSatellite, SAT_INFO_MAX_SATELLITES> entries{};
    std::optional<int> usedCount;
};
