#pragma once

#include <cstdint>

#include "GPSEllipsoidPosition.h"

/// Configuration values shared by the CFG-VALSET and pre-protocol-27 configuration paths.
namespace UBX {

// Base stations always use the stationary navigation model.
inline constexpr uint8_t STATIONARY_DYNAMIC_MODEL = 2;

/// Satellite report divisor at the 1 Hz base rate: every 2 s, as during a 5 Hz survey.
inline constexpr uint8_t BASE_SATELLITE_INFO_RATE = 2;

/// Fixed-position accuracy in 0.1 mm.
inline uint32_t fixedAccuracyWireUnits(float accuracyMeters)
{
    // Match shared validation: multiplying directly by 10000 can round an accepted value past UINT32_MAX.
    const float accuracyMillimeters = accuracyMeters * 1000.0f;
    return static_cast<uint32_t>(accuracyMillimeters * 10.0f);
}

/// Survey-in accuracy limit in 0.1 mm.
inline uint32_t surveyAccuracyWireUnits(double accuracyMeters)
{
    return static_cast<uint32_t>(accuracyMeters * 10000.0);
}

/// TMODE fixed position: latitude/longitude in 1e-7 deg and height in cm, each with a high-precision
/// remainder (1e-9 deg, 0.1 mm) in [-99, 99].
struct FixedPositionWire
{
    int32_t latitude;
    int8_t latitudeHp;
    int32_t longitude;
    int8_t longitudeHp;
    int32_t height;
    int8_t heightHp;
};

inline FixedPositionWire fixedPositionWire(const GPSEllipsoidPosition& position)
{
    const auto latitude = static_cast<int64_t>(position.latitudeDegrees * 1e9);
    const auto longitude = static_cast<int64_t>(position.longitudeDegrees * 1e9);
    const auto height = static_cast<int64_t>(static_cast<double>(position.altitudeMeters) * 1e4);
    return {static_cast<int32_t>(latitude / 100),  static_cast<int8_t>(latitude % 100),
            static_cast<int32_t>(longitude / 100), static_cast<int8_t>(longitude % 100),
            static_cast<int32_t>(height / 100),    static_cast<int8_t>(height % 100)};
}

}  // namespace UBX
