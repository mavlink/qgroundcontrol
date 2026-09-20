#pragma once

#include <cstdint>
#include <optional>

#include "GPSConstellation.h"

struct GPSNativeSatelliteData
{
    uint16_t id = 0;
    uint16_t prn = 0;
    GPSConstellation constellation = GPSConstellation::Unknown;
    std::optional<bool> used = std::nullopt;
    std::optional<double> elevation = std::nullopt;
    std::optional<double> azimuth = std::nullopt;
    std::optional<int> signal = std::nullopt;
};
