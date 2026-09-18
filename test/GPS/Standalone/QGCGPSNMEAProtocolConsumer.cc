#include <algorithm>

#include "NMEAConstellation.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentence.h"

#if defined(QT_CORE_LIB) || defined(QT_VERSION)
#error The NMEA protocol consumer must not inherit Qt dependencies.
#endif

int main()
{
    struct SatelliteCase
    {
        GPSConstellation constellation;
        int wireId;
        int expected;
    };

    constexpr SatelliteCase satelliteCases[] = {
        {GPSConstellation::GLONASS, 65, 1},  {GPSConstellation::GLONASS, 96, 32}, {GPSConstellation::GLONASS, 97, 97},
        {GPSConstellation::Galileo, 301, 1}, {GPSConstellation::BeiDou, 401, 1},  {GPSConstellation::BeiDou, 201, 1},
        {GPSConstellation::QZSS, 193, 1},    {GPSConstellation::SBAS, 33, 120},   {GPSConstellation::Unknown, 999, 999},
    };
    for (const auto& entry : satelliteCases) {
        if (gpsSatelliteId(entry.constellation, entry.wireId) != entry.expected) {
            return 7;
        }
    }
    const auto sentence = NMEA::sentence("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47");
    if (!sentence || !NMEA::gga(*sentence) || NMEA::utcMilliseconds(sentence->fields[1]) != 45319000) {
        return 1;
    }
    if (NMEA::satelliteConstellation("GP", {}, 1) != GPSConstellation::GPS) {
        return 2;
    }
    const auto view = NMEA::sentence("$GPGSV,1,1,01,01,40,083,41*43");
    if (!view) {
        return 3;
    }
    NMEA::SatelliteAssembler assembler;
    if (!assembler.ingest(*view, 1000).accepted) {
        return 4;
    }
    const auto epoch = assembler.flush();
    const auto gps = std::find_if(epoch.begin(), epoch.end(),
                                  [](const auto& system) { return system.constellation == GPSConstellation::GPS; });
    if (gps == epoch.end() || gps->inViewTimestampUs != 1000 || gps->satellites.size() != 1 ||
        gps->satellites.front().id != 1) {
        return 5;
    }
    assembler.clear();
    return assembler.flush().empty() ? 0 : 6;
}
