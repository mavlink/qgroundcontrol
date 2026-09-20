#pragma once

#include <cstdint>
#include <limits>

/// Configuration used only by the RTK base-station role.
struct GPSBaseStationConfig
{
    enum class SurveyMode
    {
        AccuracyControlled,
        ReceiverManaged,
    };

    bool useFixedBase = false;
    double surveyInAccMeters = 0.0;
    int64_t surveyInDurationSecs = 0;
    double fixedBaseLatitude = std::numeric_limits<double>::quiet_NaN();
    double fixedBaseLongitude = std::numeric_limits<double>::quiet_NaN();
    float fixedBaseAltitudeMeters = std::numeric_limits<float>::quiet_NaN();
    float fixedBaseAccuracyMeters = 0.0f;
    SurveyMode surveyMode = SurveyMode::AccuracyControlled;
    /// Maximum receiver-managed averaging time, not a minimum duration or an accuracy guarantee.
    uint32_t receiverAveragingDurationSecs = 60;

    bool operator==(const GPSBaseStationConfig&) const = default;
};
