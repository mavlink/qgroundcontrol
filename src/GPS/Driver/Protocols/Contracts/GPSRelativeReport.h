/* https://github.com/PX4/PX4-Autopilot/blob/main/msg/SensorGnssRelative.msg */

#pragma once

#include <cstdint>
#include <optional>

struct GPSRelativeReport
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
