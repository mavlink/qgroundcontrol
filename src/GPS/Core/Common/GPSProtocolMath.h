#pragma once

#include <cmath>
#include <cstdint>
#include <ctime>
#include <limits>
#include <numbers>
#include <optional>

#include <QtCore/QByteArray>

#include "GPSReceiverConfig.h"

/// Unit conversions and geodesy shared by receiver families and their callers.
namespace GPSProtocolMath {

inline constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
inline constexpr double RAD_TO_DEG = 180.0 / std::numbers::pi;
/// Receiver dates before this instant (2009-02-13) are implausible and reported as unknown.
inline constexpr time_t UTC_PLAUSIBILITY_FLOOR_SECS = static_cast<time_t>(1234567890ULL);

/// Degrees of a coordinate in 1e-7 degree units, as u-blox and MAVLink send them.
[[nodiscard]] constexpr double degreesFromE7(int32_t value)
{
    return value * 1e-7;
}

[[nodiscard]] constexpr double metersFromMillimeters(int32_t value)
{
    return value / 1000.0;
}

/// @a meters rounded to whole millimetres.
[[nodiscard]] inline double roundedToMillimeters(double meters)
{
    return static_cast<double>(std::llround(meters * 1000.0)) / 1000.0;
}

/// @a meters in 0.1 mm, truncated toward zero. Empty when not finite, negative or beyond a 32-bit unsigned field.
[[nodiscard]] inline std::optional<uint32_t> tenthMillimeters(double meters)
{
    const double units = meters * 10000.0;
    if (!std::isfinite(units) || units < 0 || units > (std::numeric_limits<uint32_t>::max)()) {
        return std::nullopt;
    }
    return static_cast<uint32_t>(units);
}

struct Ecef
{
    double x = 0;
    double y = 0;
    double z = 0;
};

[[nodiscard]] Ecef toEcef(const GPSEllipsoidPosition& position);
[[nodiscard]] GPSEllipsoidPosition fromEcef(const Ecef& position);

/// Straight-line distance between @a left and @a right, in metres.
[[nodiscard]] inline double distance(const Ecef& left, const Ecef& right)
{
    return std::hypot(left.x - right.x, left.y - right.y, left.z - right.z);
}

/// Whether @a position lies 6000 to 7000 km from the Earth's centre, as a receiver's coordinates do; false for the
/// zero or unknown coordinates of an unset base.
[[nodiscard]] bool nearEarthSurface(const Ecef& position);

/// Microseconds since the Unix epoch of a receiver's UTC calendar fields plus @a nsec, which may be negative. Fields
/// beyond their range carry into the next larger unit, as a leap second's 60 does. 0 when the date is implausible or
/// beyond the platform's time_t.
[[nodiscard]] uint64_t utcMicroseconds(int year, int month, int day, int hour, int minute, int second, int32_t nsec);

}  // namespace GPSProtocolMath

/// Formats @a value as printf's "%.<decimals>f" does in the C locale: exact binary ties round to even and a negative
/// zero keeps its sign. QByteArray::number() does neither, and would change the command bytes for such values.
[[nodiscard]] QByteArray gpsFixedDecimal(double value, int decimals);
