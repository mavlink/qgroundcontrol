#pragma once

#include <cstdint>
#include <limits>
#include <optional>

#include "GPSDriverReports.h"

struct GPSNativePositionReport
{
    GPSNavigationValues navigation{};
    float speedAccuracyMetersPerSecond = std::numeric_limits<float>::quiet_NaN();
    float courseAccuracyRadians = std::numeric_limits<float>::quiet_NaN();

    uint64_t dop_timestamp{};
    uint64_t heading_timestamp{};
    uint64_t accuracy_timestamp{};
    float vel_n_m_s = std::numeric_limits<float>::quiet_NaN();
    float vel_e_m_s = std::numeric_limits<float>::quiet_NaN();
    float vel_d_m_s = std::numeric_limits<float>::quiet_NaN();
    bool vel_ned_valid{};

    int32_t timestamp_time_relative{};
};
