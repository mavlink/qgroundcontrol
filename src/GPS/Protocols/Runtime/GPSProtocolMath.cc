#include "GPSProtocolMath.h"

#include "GPSProtocolTime.h"
#include <GeographicLib/Geocentric.hpp>

namespace GPSProtocolMath {

Ecef toEcef(const GPSEllipsoidPosition& position)
{
    Ecef result;
    GeographicLib::Geocentric::WGS84().Forward(position.latitudeDegrees, position.longitudeDegrees,
                                               position.altitudeMeters, result.x, result.y, result.z);
    return result;
}

GPSEllipsoidPosition fromEcef(const Ecef& position)
{
    GPSEllipsoidPosition result;
    double height = 0;
    GeographicLib::Geocentric::WGS84().Reverse(position.x, position.y, position.z, result.latitudeDegrees,
                                               result.longitudeDegrees, height);
    result.altitudeMeters = static_cast<float>(height);
    return result;
}

uint64_t utcMicroseconds(tm& utc, int32_t nsec)
{
    const time_t epoch = gpsTimeToEpoch(utc);
    if (epoch > UTC_PLAUSIBILITY_FLOOR_SECS) {
        return static_cast<uint64_t>(epoch) * 1000000ULL + nsec / 1000;
    }
    return 0;
}

}  // namespace GPSProtocolMath
