#pragma once

#include <optional>
#include <string_view>

#include "GPSConstellation.h"

namespace NMEA {
inline GPSConstellation satelliteConstellation(std::string_view talker, std::optional<int> systemId,
                                               std::optional<int> satelliteId)
{
    using Constellation = GPSConstellation;
    if ((talker == "GP" || (talker == "GN" && (!systemId || *systemId == 1))) && satelliteId &&
        ((*satelliteId >= 33 && *satelliteId <= 64) || (*satelliteId >= 120 && *satelliteId <= 158)))
        return Constellation::SBAS;
    if (talker == "GP")
        return Constellation::GPS;
    if (talker == "GL")
        return Constellation::GLONASS;
    if (talker == "GA")
        return Constellation::Galileo;
    if (talker == "GB" || talker == "BD")
        return Constellation::BeiDou;
    if (talker == "GQ" || talker == "PQ" || talker == "QZ")
        return Constellation::QZSS;
    if (talker != "GN")
        return Constellation::Unknown;
    if (systemId) {
        switch (*systemId) {
            case 1:
                return Constellation::GPS;
            case 2:
                return Constellation::GLONASS;
            case 3:
                return Constellation::Galileo;
            case 4:
                return Constellation::BeiDou;
            case 5:
                return Constellation::QZSS;
            default:
                return Constellation::Unknown;
        }
    }
    const int id = satelliteId.value_or(0);
    // Keep the established GP bucket for GPS/SBAS, matching constellation-specific GSA/GSV.
    if ((id >= 1 && id <= 64) || (id >= 152 && id <= 158))
        return Constellation::GPS;
    if (id >= 65 && id <= 96)
        return Constellation::GLONASS;
    if (id >= 193 && id <= 200)
        return Constellation::QZSS;
    // Qt's legacy BeiDou range starts at 201; u-blox extended QZSS ends at 202.
    // Without explicit context, neither interpretation is safe for 201/202.
    if ((id >= 203 && id <= 235) || (id >= 401 && id <= 463))
        return Constellation::BeiDou;
    if (id >= 301 && id <= 336)
        return Constellation::Galileo;
    return Constellation::Unknown;
}

}  // namespace NMEA
