#pragma once

#include <optional>
#include <string_view>

#include "GPSConstellation.h"

namespace NMEA {
GPSConstellation satelliteConstellation(std::string_view talker, std::optional<int> systemId,
                                        std::optional<int> satelliteId);

}  // namespace NMEA
