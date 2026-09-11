#pragma once

/// Configuration used only by the RTK base-station role.
struct GPSBaseStationConfig
{
    bool operator==(const GPSBaseStationConfig&) const = default;
    bool useFixedBase = false;
    double surveyInAccMeters = 0.0;
    int surveyInDurationSecs = 0;
    double fixedBaseLatitude = 0.0;
    double fixedBaseLongitude = 0.0;
    float fixedBaseAltitudeMeters = 0.0f;
    float fixedBaseAccuracyMeters = 0.0f;
};
