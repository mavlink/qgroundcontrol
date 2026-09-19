#pragma once

#include <cstdint>
#include <optional>

struct GPSNativeIntegrityReport
{
    uint64_t timestamp = 0;
    std::optional<int32_t> noise_per_ms = std::nullopt;
    std::optional<uint16_t> automatic_gain_control = std::nullopt;

    static constexpr uint8_t JAMMING_STATE_UNKNOWN = 0;
    static constexpr uint8_t JAMMING_STATE_OK = 1;
    static constexpr uint8_t JAMMING_STATE_MITIGATED = 2;
    static constexpr uint8_t JAMMING_STATE_DETECTED = 3;
    uint8_t jamming_state{};
    uint64_t jamming_state_timestamp{};
    std::optional<int32_t> jamming_indicator = std::nullopt;
    uint64_t rf_timestamp = 0;

    static constexpr uint8_t SPOOFING_STATE_UNKNOWN = 0;
    static constexpr uint8_t SPOOFING_STATE_OK = 1;
    static constexpr uint8_t SPOOFING_STATE_MITIGATED = 2;
    static constexpr uint8_t SPOOFING_STATE_DETECTED = 3;
    uint8_t spoofing_state{};
    uint64_t spoofing_state_timestamp{};

    static constexpr uint8_t CORRECTIONS_PROTOCOL_UNKNOWN = 0;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_RTCM3 = 1;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_SPARTN = 2;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_HAS = 3;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_PMP = 4;
    static constexpr uint8_t CORRECTIONS_PROTOCOL_QZSS_L6 = 5;
    uint8_t corrections_protocol{};

    std::optional<bool> corrections_crc_failed = std::nullopt;

    static constexpr uint8_t CORRECTIONS_MSG_USED_UNKNOWN = 0;
    static constexpr uint8_t CORRECTIONS_MSG_USED_NOT_USED = 1;
    static constexpr uint8_t CORRECTIONS_MSG_USED_USED = 2;
    uint8_t corrections_msg_used{};
    uint64_t corrections_timestamp{};
};
