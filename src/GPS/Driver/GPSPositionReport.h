#pragma once

#include <cstdint>
#include <optional>

struct GPSPositionReport
{
    uint64_t timestamp{};

    double latitude_deg{};
    double longitude_deg{};
    double altitude_msl_m{};
    double altitude_ellipsoid_m{};

    float s_variance_m_s{};
    float c_variance_rad{};

    static constexpr uint8_t FIX_TYPE_NONE = 1;
    static constexpr uint8_t FIX_TYPE_2D = 2;
    static constexpr uint8_t FIX_TYPE_3D = 3;
    static constexpr uint8_t FIX_TYPE_RTCM_CODE_DIFFERENTIAL = 4;
    static constexpr uint8_t FIX_TYPE_RTK_FLOAT = 5;
    static constexpr uint8_t FIX_TYPE_RTK_FIXED = 6;
    static constexpr uint8_t FIX_TYPE_EXTRAPOLATED = 8;
    uint8_t fix_type{};

    float eph{};
    float epv{};

    uint64_t dop_timestamp{};
    uint64_t heading_timestamp{};
    uint64_t accuracy_timestamp{};
    float hdop{};
    float vdop{};

    std::optional<int32_t> noise_per_ms;
    std::optional<uint16_t> automatic_gain_control;

    static constexpr uint8_t JAMMING_STATE_UNKNOWN = 0;
    static constexpr uint8_t JAMMING_STATE_OK = 1;
    static constexpr uint8_t JAMMING_STATE_MITIGATED = 2;
    static constexpr uint8_t JAMMING_STATE_DETECTED = 3;
    uint8_t jamming_state{};
    uint64_t jamming_state_timestamp{};
    std::optional<int32_t> jamming_indicator;
    uint64_t rf_timestamp = 0;

    static constexpr uint8_t SPOOFING_STATE_UNKNOWN = 0;
    static constexpr uint8_t SPOOFING_STATE_OK = 1;
    static constexpr uint8_t SPOOFING_STATE_MITIGATED = 2;
    static constexpr uint8_t SPOOFING_STATE_DETECTED = 3;
    uint8_t spoofing_state{};
    uint64_t spoofing_state_timestamp{};

    static constexpr uint8_t AUTHENTICATION_STATE_UNKNOWN = 0;
    static constexpr uint8_t AUTHENTICATION_STATE_INITIALIZING = 1;
    static constexpr uint8_t AUTHENTICATION_STATE_ERROR = 2;
    static constexpr uint8_t AUTHENTICATION_STATE_OK = 3;
    static constexpr uint8_t AUTHENTICATION_STATE_DISABLED = 4;
    uint8_t authentication_state{};
    uint64_t authentication_state_timestamp{};

    float vel_m_s{};
    float vel_n_m_s{};
    float vel_e_m_s{};
    float vel_d_m_s{};
    float cog_rad{};
    bool vel_ned_valid{};

    int32_t timestamp_time_relative{};
    uint64_t time_utc_usec{};

    uint8_t satellites_used{};

    static constexpr uint32_t SYSTEM_ERROR_OK = 0;
    static constexpr uint32_t SYSTEM_ERROR_INCOMING_CORRECTIONS = 1;
    static constexpr uint32_t SYSTEM_ERROR_CONFIGURATION = 2;
    static constexpr uint32_t SYSTEM_ERROR_SOFTWARE = 4;
    static constexpr uint32_t SYSTEM_ERROR_ANTENNA = 8;
    static constexpr uint32_t SYSTEM_ERROR_EVENT_CONGESTION = 16;
    static constexpr uint32_t SYSTEM_ERROR_CPU_OVERLOAD = 32;
    static constexpr uint32_t SYSTEM_ERROR_OUTPUT_CONGESTION = 64;
    uint32_t system_error{};

    float heading{};
    float heading_accuracy{};

    static constexpr uint8_t CORRECTIONS_PROTOCOL_UNKNOWN = 0;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_RTCM3 = 1;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_SPARTN = 2;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_HAS = 3;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_PMP = 4;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_QZSS_L6 = 5;
    uint8_t corrections_protocol{};

    std::optional<bool> corrections_crc_failed;

    static constexpr uint8_t CORRECTIONS_MSG_USED_UNKNOWN = 0;
    static constexpr uint8_t CORRECTIONS_MSG_USED_NOT_USED = 1;
    static constexpr uint8_t CORRECTIONS_MSG_USED_USED = 2;
    uint8_t corrections_msg_used{};
    uint64_t corrections_timestamp{};
};
