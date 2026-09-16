#pragma once

#include <cstdint>

/// Configuration used only by the RTK base-station role.
struct GPSBaseStationConfig
{
    bool useFixedBase = false;
    double surveyInAccMeters = 0.0;
    int64_t surveyInDurationSecs = 0;
    double fixedBaseLatitude = 0.0;
    double fixedBaseLongitude = 0.0;
    float fixedBaseAltitudeMeters = 0.0f;
    float fixedBaseAccuracyMeters = 0.0f;

    bool operator==(const GPSBaseStationConfig&) const = default;
};
