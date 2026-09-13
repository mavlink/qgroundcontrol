#pragma once

#include <cstdint>
#include <limits>
#include <optional>

struct GPSPositionReport
{
    uint64_t timestamp{};

    double latitude_deg = std::numeric_limits<double>::quiet_NaN();
    double longitude_deg = std::numeric_limits<double>::quiet_NaN();
    double altitude_msl_m = std::numeric_limits<double>::quiet_NaN();
    double altitude_ellipsoid_m = std::numeric_limits<double>::quiet_NaN();

    float speedAccuracyMetersPerSecond = std::numeric_limits<float>::quiet_NaN();
    float courseAccuracyRadians = std::numeric_limits<float>::quiet_NaN();

    static constexpr uint8_t FIX_TYPE_NONE = 1;
    static constexpr uint8_t FIX_TYPE_2D = 2;
    static constexpr uint8_t FIX_TYPE_3D = 3;
    static constexpr uint8_t FIX_TYPE_RTCM_CODE_DIFFERENTIAL = 4;
    static constexpr uint8_t FIX_TYPE_RTK_FLOAT = 5;
    static constexpr uint8_t FIX_TYPE_RTK_FIXED = 6;
    static constexpr uint8_t FIX_TYPE_EXTRAPOLATED = 8;
    uint8_t fix_type{};

    float eph = std::numeric_limits<float>::quiet_NaN();
    float epv = std::numeric_limits<float>::quiet_NaN();

    uint64_t dop_timestamp{};
    uint64_t heading_timestamp{};
    uint64_t accuracy_timestamp{};
    float hdop = std::numeric_limits<float>::quiet_NaN();
    float vdop = std::numeric_limits<float>::quiet_NaN();

    float vel_m_s = std::numeric_limits<float>::quiet_NaN();
    float vel_n_m_s = std::numeric_limits<float>::quiet_NaN();
    float vel_e_m_s = std::numeric_limits<float>::quiet_NaN();
    float vel_d_m_s = std::numeric_limits<float>::quiet_NaN();
    float cog_rad = std::numeric_limits<float>::quiet_NaN();
    bool vel_ned_valid{};

    int32_t timestamp_time_relative{};
    uint64_t time_utc_usec{};

    uint8_t satellites_used = std::numeric_limits<uint8_t>::max();

    float heading = std::numeric_limits<float>::quiet_NaN();
    float heading_accuracy = std::numeric_limits<float>::quiet_NaN();
};
