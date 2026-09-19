#include <algorithm>
#include <cmath>
#include <limits>

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
        {GPSConstellation::GLONASS, 64, 64},   {GPSConstellation::GLONASS, 65, 1},
        {GPSConstellation::GLONASS, 96, 32},   {GPSConstellation::GLONASS, 97, 97},
        {GPSConstellation::Galileo, 300, 300}, {GPSConstellation::Galileo, 301, 1},
        {GPSConstellation::Galileo, 336, 36},  {GPSConstellation::Galileo, 337, 337},
        {GPSConstellation::BeiDou, 400, 400},  {GPSConstellation::BeiDou, 401, 1},
        {GPSConstellation::BeiDou, 463, 63},   {GPSConstellation::BeiDou, 464, 464},
        {GPSConstellation::BeiDou, 200, 200},  {GPSConstellation::BeiDou, 201, 1},
        {GPSConstellation::BeiDou, 202, 2},    {GPSConstellation::BeiDou, 235, 35},
        {GPSConstellation::BeiDou, 236, 236},  {GPSConstellation::QZSS, 192, 192},
        {GPSConstellation::QZSS, 193, 1},      {GPSConstellation::QZSS, 201, 9},
        {GPSConstellation::QZSS, 202, 10},     {GPSConstellation::QZSS, 203, 203},
        {GPSConstellation::SBAS, 32, 32},      {GPSConstellation::SBAS, 33, 120},
        {GPSConstellation::SBAS, 64, 151},     {GPSConstellation::SBAS, 65, 65},
        {GPSConstellation::SBAS, 120, 120},    {GPSConstellation::GPS, 33, 33},
        {GPSConstellation::NavIC, 401, 401},   {GPSConstellation::Unknown, 193, 193},
        {GPSConstellation::Unknown, 999, 999},
    };
    for (const auto& entry : satelliteCases) {
        if (NMEA::satelliteId(entry.constellation, entry.wireId) != entry.expected) {
            return 7;
        }
    }

    struct CoordinateCase
    {
        double degreesMinutes;
        double expected;
    };

    constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();
    constexpr double INFINITY_VALUE = std::numeric_limits<double>::infinity();
    constexpr CoordinateCase coordinateCases[] = {
        {0.0, 0.0},
        {-0.0, 0.0},
        {30.0, 0.5},
        {-30.0, -0.5},
        {4807.038, 48.1173},
        {-4807.038, -48.1173},
        {18000.0, 180.0},
        {-18000.0, -180.0},
        {18000.001, NAN_VALUE},
        {-18000.001, NAN_VALUE},
        {1260.0, NAN_VALUE},
        {-1260.0, NAN_VALUE},
        {NAN_VALUE, NAN_VALUE},
        {INFINITY_VALUE, NAN_VALUE},
        {-INFINITY_VALUE, NAN_VALUE},
    };
    for (const auto& entry : coordinateCases) {
        const double actual = NMEA::degreesFromDegreesMinutes(entry.degreesMinutes);
        if (std::isnan(entry.expected)) {
            if (!std::isnan(actual)) {
                return 8;
            }
        } else if (!std::isfinite(actual) || std::abs(actual - entry.expected) > 1e-10) {
            return 8;
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
