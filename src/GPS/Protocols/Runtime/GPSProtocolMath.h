#pragma once

#include <cstdint>
#include <ctime>
#include <numbers>

#include "GPSEllipsoidPosition.h"

/// Unit conversions and geodesy shared by receiver families.
namespace GPSProtocolMath {

inline constexpr float PI = std::numbers::pi_v<float>;
inline constexpr float DEG_TO_RAD = PI / 180.0f;
inline constexpr double RAD_TO_DEG = 180.0 / std::numbers::pi;
/// Receiver dates before this instant (2009-02-13) are implausible and reported as unknown.
inline constexpr time_t UTC_PLAUSIBILITY_FLOOR_SECS = static_cast<time_t>(1234567890ULL);

struct Ecef
{
    double x = 0;
    double y = 0;
    double z = 0;
};

[[nodiscard]] Ecef toEcef(const GPSEllipsoidPosition& position);
[[nodiscard]] GPSEllipsoidPosition fromEcef(const Ecef& position);

/// Microseconds since the Unix epoch of broken-down UTC @a utc (normalised in place) plus @a nsec, which may be
/// negative; 0 when the date is implausible.
[[nodiscard]] uint64_t utcMicroseconds(tm& utc, int32_t nsec);

}  // namespace GPSProtocolMath
