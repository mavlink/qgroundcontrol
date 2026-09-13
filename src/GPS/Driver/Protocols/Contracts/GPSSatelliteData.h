#pragma once

#include <cstdint>
#include <optional>

#include "GPSConstellation.h"

struct GPSSatelliteData
{
    uint16_t id = 0;
    uint16_t prn = 0;
    GPSConstellation constellation = GPSConstellation::Unknown;
    std::optional<bool> used;
    std::optional<double> elevation;
    std::optional<double> azimuth;
    std::optional<int> signal;
};
