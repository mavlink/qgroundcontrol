#pragma once

#include <cstdint>
#include <variant>

#include "GPSEllipsoidPosition.h"

/// Configuration used only by the RTK base-station role.
struct GPSBaseStationConfig
{
    struct SurveyIn
    {
        double accuracyMeters = 0.0;
        int64_t durationSecs = 0;
        bool operator==(const SurveyIn&) const = default;
    };

    struct Fixed
    {
        GPSEllipsoidPosition position{};
        float accuracyMeters = 0.0f;
        bool operator==(const Fixed&) const = default;
    };

    struct ReceiverAveraging
    {
        /// Maximum receiver-managed time, not a minimum duration or an accuracy guarantee.
        uint32_t maximumDurationSecs = 60;
        bool operator==(const ReceiverAveraging&) const = default;
    };

    using Mode = std::variant<SurveyIn, Fixed, ReceiverAveraging>;
    Mode mode = SurveyIn{};
    /// MSM4 instead of MSM7 observations: about a third less correction bandwidth, without Doppler.
    bool compactObservations = false;

    bool operator==(const GPSBaseStationConfig&) const = default;
};
