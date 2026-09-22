#pragma once

#include <array>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "GPSDriverReports.h"
#include "GPSNativePositionReport.h"
#include "GPSNativeSatelliteReport.h"
#include "GPSNativeSurveyReport.h"

/* https://github.com/PX4/PX4-Autopilot/blob/main/msg/SensorGnssRelative.msg */
struct GPSNativeRelativeReport
{
    uint64_t timestamp;
    uint64_t timestamp_sample;

    uint64_t time_utc_usec;

    std::optional<uint16_t> reference_station_id;

    float position[3];
    float position_accuracy[3];

    float heading;
    float heading_accuracy;

    float position_length;
    float accuracy_length;

    bool gnss_fix_ok;
    bool differential_solution;
    bool relative_position_valid;
    bool carrier_solution_floating;
    bool carrier_solution_fixed;
    std::optional<bool> moving_base_mode;
    std::optional<bool> reference_position_miss;
    std::optional<bool> reference_observations_miss;
    bool heading_valid;
    std::optional<bool> relative_position_normalized;
};

struct GPSRTCMReport
{
    std::array<uint8_t, 1029> bytes{};
    size_t size = 0;
};

using GPSDecodedEvent =
    std::variant<GPSNativePositionReport, GPSIntegrityReport, GPSNativeSatelliteReport, GPSSatelliteUsageReport,
                 GPSNativeRelativeReport, GPSNativeSurveyReport, GPSRTCMReport>;

/// A bounded, owned sequence. The caller decides whether position epochs may be coalesced.
struct GPSDecodedBatch
{
    static constexpr size_t MAX_EVENTS = 8;
    static constexpr int PROTOCOL_ACTIVITY = 4;
    std::vector<GPSDecodedEvent> events;
    int updates = 0;
};

struct GPSDecodeResult
{
    size_t bytesConsumed = 0;
    GPSDecodedBatch batch;
};
