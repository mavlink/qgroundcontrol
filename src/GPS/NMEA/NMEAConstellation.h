#pragma once

#include <optional>
#include <string_view>

#include "GPSConstellation.h"

namespace NMEA {
namespace SatelliteIds {
inline constexpr int FIRST_LOCAL_ID = 1;
inline constexpr int GLONASS_FIRST_NMEA_ID = 65;
inline constexpr int GLONASS_LAST_NMEA_ID = 96;
inline constexpr int GALILEO_FIRST_NMEA_ID = 301;
inline constexpr int GALILEO_LAST_NMEA_ID = 336;
inline constexpr int BEIDOU_EXTENDED_FIRST_NMEA_ID = 401;
inline constexpr int BEIDOU_EXTENDED_LAST_NMEA_ID = 463;
inline constexpr int BEIDOU_LEGACY_FIRST_NMEA_ID = 201;
inline constexpr int BEIDOU_LEGACY_LAST_NMEA_ID = 235;
inline constexpr int QZSS_FIRST_NMEA_ID = 193;
inline constexpr int QZSS_LAST_NMEA_ID = 202;
inline constexpr int SBAS_FIRST_NMEA_ID = 33;
inline constexpr int SBAS_LAST_NMEA_ID = 64;
inline constexpr int SBAS_FIRST_PRN = 120;
}  // namespace SatelliteIds

/// Canonical constellation-local identifier; preserve unknown/vendor ranges unchanged.
inline int satelliteId(GPSConstellation constellation, int wireId)
{
    namespace Id = SatelliteIds;

    switch (constellation) {
        case GPSConstellation::GLONASS:
            return wireId >= Id::GLONASS_FIRST_NMEA_ID && wireId <= Id::GLONASS_LAST_NMEA_ID
                       ? wireId - Id::GLONASS_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID
                       : wireId;
        case GPSConstellation::Galileo:
            return wireId >= Id::GALILEO_FIRST_NMEA_ID && wireId <= Id::GALILEO_LAST_NMEA_ID
                       ? wireId - Id::GALILEO_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID
                       : wireId;
        case GPSConstellation::BeiDou:
            if (wireId >= Id::BEIDOU_EXTENDED_FIRST_NMEA_ID && wireId <= Id::BEIDOU_EXTENDED_LAST_NMEA_ID) {
                return wireId - Id::BEIDOU_EXTENDED_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID;
            }
            return wireId >= Id::BEIDOU_LEGACY_FIRST_NMEA_ID && wireId <= Id::BEIDOU_LEGACY_LAST_NMEA_ID
                       ? wireId - Id::BEIDOU_LEGACY_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID
                       : wireId;
        case GPSConstellation::QZSS:
            return wireId >= Id::QZSS_FIRST_NMEA_ID && wireId <= Id::QZSS_LAST_NMEA_ID
                       ? wireId - Id::QZSS_FIRST_NMEA_ID + Id::FIRST_LOCAL_ID
                       : wireId;
        case GPSConstellation::SBAS:
            return wireId >= Id::SBAS_FIRST_NMEA_ID && wireId <= Id::SBAS_LAST_NMEA_ID
                       ? wireId - Id::SBAS_FIRST_NMEA_ID + Id::SBAS_FIRST_PRN
                       : wireId;
        default:
            return wireId;
    }
}

GPSConstellation satelliteConstellation(std::string_view talker, std::optional<int> systemId,
                                        std::optional<int> satelliteId);

}  // namespace NMEA
