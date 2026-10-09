#include "GPSProtocolMath.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

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
    result.altitudeMeters = height;
    return result;
}

bool nearEarthSurface(const Ecef& position)
{
    const double radius = std::hypot(position.x, position.y, position.z);
    return radius >= 6000000 && radius <= 7000000;
}

uint64_t utcMicroseconds(int year, int month, int day, int hour, int minute, int second, int32_t nsec)
{
    using namespace std::chrono;
    const int64_t monthIndex = static_cast<int64_t>(year) * 12 + (month - 1);
    const int64_t normalizedYear = monthIndex / 12 - (monthIndex % 12 < 0 ? 1 : 0);
    if (normalizedYear < int(std::chrono::year::min()) || normalizedYear > int(std::chrono::year::max())) {
        return 0;
    }
    const auto normalizedMonth = static_cast<unsigned>((monthIndex % 12 + 12) % 12 + 1);
    const sys_seconds instant =
        sys_days(std::chrono::year(static_cast<int>(normalizedYear)) / std::chrono::month(normalizedMonth) / 1) +
        days(day - 1) + hours(hour) + minutes(minute) + seconds(second);
    const auto date = floor<days>(instant);
    if (date < sys_days(std::chrono::year::min() / January / 1) ||
        date > sys_days(std::chrono::year::max() / December / 31)) {
        return 0;
    }
    const int64_t epoch = instant.time_since_epoch().count();
    if (epoch <= UTC_PLAUSIBILITY_FLOOR_SECS ||
        static_cast<uint64_t>(epoch) > static_cast<uint64_t>((std::numeric_limits<time_t>::max)())) {
        return 0;
    }
    return static_cast<uint64_t>(epoch) * 1000000ULL + nsec / 1000;
}

}  // namespace GPSProtocolMath

QByteArray gpsFixedDecimal(double value, int decimals)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << std::fixed << std::setprecision(decimals) << value;
    return QByteArray::fromStdString(text.str());
}
