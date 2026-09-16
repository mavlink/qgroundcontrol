#include <cstdlib>
#include <iostream>
#include <limits>

#include "GPSAltitudeDatum.h"
#include "GPSBaseStationConfig.h"
#include "GPSConstellation.h"
#include "GPSIOStatus.h"

int main()
{
    static_assert(static_cast<int>(GPSAltitudeDatum::Unknown) == 0);
    static_assert(static_cast<int>(GPSAltitudeDatum::MeanSeaLevel) == 1);
    static_assert(static_cast<int>(GPSAltitudeDatum::Ellipsoid) == 2);
    static_assert(static_cast<int>(GPSOpenStatus::Opened) == 0);
    static_assert(static_cast<int>(GPSOpenStatus::TimedOut) == 1);
    static_assert(static_cast<int>(GPSOpenStatus::Cancelled) == 2);
    static_assert(static_cast<int>(GPSOpenStatus::Error) == 3);
    static_assert(static_cast<int>(GPSOpenStatus::Unsupported) == 4);
    static_assert(static_cast<int>(GPSReadStatus::Data) == 0);
    static_assert(static_cast<int>(GPSReadStatus::TimedOut) == 1);
    static_assert(static_cast<int>(GPSReadStatus::Cancelled) == 2);
    static_assert(static_cast<int>(GPSReadStatus::Closed) == 3);
    static_assert(static_cast<int>(GPSReadStatus::Error) == 4);
    static_assert(static_cast<int>(GPSReadStatus::Overflow) == 5);
    static_assert(static_cast<int>(GPSReadStatus::InvalidData) == 6);
    static_assert(static_cast<int>(GPSWriteStatus::Completed) == 0);
    static_assert(static_cast<int>(GPSWriteStatus::TimedOut) == 1);
    static_assert(static_cast<int>(GPSWriteStatus::Cancelled) == 2);
    static_assert(static_cast<int>(GPSWriteStatus::Error) == 3);
    static_assert(static_cast<int>(GPSWriteStatus::Unsupported) == 4);
    static_assert(static_cast<int>(GPSWriteStatus::InvalidData) == 5);
    const auto require = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
        }
        return condition;
    };
    const GPSBaseStationConfig base{.surveyInDurationSecs = (std::numeric_limits<uint32_t>::max)()};
    if (!require(base.surveyInDurationSecs == 4294967295LL, "Survey duration lost legacy wire range")) {
        return EXIT_FAILURE;
    }
    struct SatelliteCase
    {
        GPSConstellation constellation;
        int wireId;
        int expected;
    };

    constexpr SatelliteCase cases[] = {
        {GPSConstellation::GLONASS, 65, 1},  {GPSConstellation::GLONASS, 96, 32}, {GPSConstellation::GLONASS, 97, 97},
        {GPSConstellation::Galileo, 301, 1}, {GPSConstellation::BeiDou, 401, 1},  {GPSConstellation::BeiDou, 201, 1},
        {GPSConstellation::QZSS, 193, 1},    {GPSConstellation::SBAS, 33, 120},   {GPSConstellation::Unknown, 999, 999},
    };
    for (const auto& entry : cases) {
        if (!require(gpsSatelliteId(entry.constellation, entry.wireId) == entry.expected,
                     "Constellation-local satellite identity changed")) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
