#include <algorithm>

#include "NMEAConstellation.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentence.h"

#ifdef QT_CORE_LIB
#error The NMEA protocol consumer must not inherit Qt dependencies.
#endif

int main()
{
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
