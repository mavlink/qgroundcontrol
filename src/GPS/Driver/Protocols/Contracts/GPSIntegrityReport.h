#pragma once

#include <cstdint>
#include <optional>

struct GPSIntegrityReport
{
    uint64_t timestamp = 0;
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

    static constexpr uint32_t SYSTEM_ERROR_OK = 0;
    static constexpr uint32_t SYSTEM_ERROR_INCOMING_CORRECTIONS = 1;
    static constexpr uint32_t SYSTEM_ERROR_CONFIGURATION = 2;
    static constexpr uint32_t SYSTEM_ERROR_SOFTWARE = 4;
    static constexpr uint32_t SYSTEM_ERROR_ANTENNA = 8;
    static constexpr uint32_t SYSTEM_ERROR_EVENT_CONGESTION = 16;
    static constexpr uint32_t SYSTEM_ERROR_CPU_OVERLOAD = 32;
    static constexpr uint32_t SYSTEM_ERROR_OUTPUT_CONGESTION = 64;
    uint32_t system_error{};

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
