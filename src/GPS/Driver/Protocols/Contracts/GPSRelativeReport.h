/* https://github.com/PX4/PX4-Autopilot/blob/main/msg/SensorGnssRelative.msg */

#pragma once

#include <cstdint>
#include <limits>
#include <optional>

struct GPSRelativeReport
{
    uint64_t timestamp = 0;
    uint64_t timestamp_sample = 0;

    uint64_t time_utc_usec = 0;

    std::optional<uint16_t> reference_station_id;

    float position[3] = {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
                         std::numeric_limits<float>::quiet_NaN()};
    float position_accuracy[3] = {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
                                  std::numeric_limits<float>::quiet_NaN()};

    float heading = std::numeric_limits<float>::quiet_NaN();
    float heading_accuracy = std::numeric_limits<float>::quiet_NaN();

    float position_length = std::numeric_limits<float>::quiet_NaN();
    float accuracy_length = std::numeric_limits<float>::quiet_NaN();

    bool gnss_fix_ok = false;
    bool differential_solution = false;
    bool relative_position_valid = false;
    bool carrier_solution_floating = false;
    bool carrier_solution_fixed = false;
    std::optional<bool> moving_base_mode;
    std::optional<bool> reference_position_miss;
    std::optional<bool> reference_observations_miss;
    bool heading_valid = false;
    std::optional<bool> relative_position_normalized;
};
