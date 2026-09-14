#include "NMEAConstellation.h"

namespace NMEA {
GPSConstellation satelliteConstellation(std::string_view talker, std::optional<int> systemId,
                                        std::optional<int> satelliteId)
{
    using Constellation = GPSConstellation;
    namespace Id = GPSSatelliteIds;
    constexpr int GPS_SYSTEM_ID = 1;
    constexpr int GLONASS_SYSTEM_ID = 2;
    constexpr int GALILEO_SYSTEM_ID = 3;
    constexpr int BEIDOU_SYSTEM_ID = 4;
    constexpr int QZSS_SYSTEM_ID = 5;
    constexpr int NAVIC_SYSTEM_ID = 6;
    constexpr int SBAS_LAST_PRN = 158;
    constexpr int LEGACY_SBAS_FIRST_PRN = 152;
    constexpr int QZSS_LEGACY_LAST_NMEA_ID = 200;
    constexpr int BEIDOU_UNAMBIGUOUS_FIRST_NMEA_ID = Id::QZSS_LAST_NMEA_ID + 1;

    if ((talker == "GP" || (talker == "GN" && (!systemId || *systemId == GPS_SYSTEM_ID))) && satelliteId &&
        ((*satelliteId >= Id::SBAS_FIRST_NMEA_ID && *satelliteId <= Id::SBAS_LAST_NMEA_ID) ||
         (*satelliteId >= Id::SBAS_FIRST_PRN && *satelliteId <= SBAS_LAST_PRN)))
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
    if (talker == "GI")
        return Constellation::NavIC;
    if (talker != "GN")
        return Constellation::Unknown;
    if (systemId) {
        switch (*systemId) {
            case GPS_SYSTEM_ID:
                return Constellation::GPS;
            case GLONASS_SYSTEM_ID:
                return Constellation::GLONASS;
            case GALILEO_SYSTEM_ID:
                return Constellation::Galileo;
            case BEIDOU_SYSTEM_ID:
                return Constellation::BeiDou;
            case QZSS_SYSTEM_ID:
                return Constellation::QZSS;
            case NAVIC_SYSTEM_ID:
                return Constellation::NavIC;
            default:
                return Constellation::Unknown;
        }
    }
    if (!satelliteId) {
        return Constellation::Unknown;
    }
    const int id = *satelliteId;
    // Keep the established GP bucket for GPS/SBAS, matching constellation-specific GSA/GSV.
    if ((id >= Id::FIRST_LOCAL_ID && id <= Id::SBAS_LAST_NMEA_ID) ||
        (id >= LEGACY_SBAS_FIRST_PRN && id <= SBAS_LAST_PRN))
        return Constellation::GPS;
    if (id >= Id::GLONASS_FIRST_NMEA_ID && id <= Id::GLONASS_LAST_NMEA_ID)
        return Constellation::GLONASS;
    if (id >= Id::QZSS_FIRST_NMEA_ID && id <= QZSS_LEGACY_LAST_NMEA_ID)
        return Constellation::QZSS;
    // Qt's legacy BeiDou range starts at 201; u-blox extended QZSS ends at 202.
    // Without explicit context, neither interpretation is safe for 201/202.
    if ((id >= BEIDOU_UNAMBIGUOUS_FIRST_NMEA_ID && id <= Id::BEIDOU_LEGACY_LAST_NMEA_ID) ||
        (id >= Id::BEIDOU_EXTENDED_FIRST_NMEA_ID && id <= Id::BEIDOU_EXTENDED_LAST_NMEA_ID))
        return Constellation::BeiDou;
    if (id >= Id::GALILEO_FIRST_NMEA_ID && id <= Id::GALILEO_LAST_NMEA_ID)
        return Constellation::Galileo;
    return Constellation::Unknown;
}
}  // namespace NMEA
