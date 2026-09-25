#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <span>
#include <tuple>

#include "UBXMessages.h"
#include "WireFields.h"

// Wire offsets and sizes, checked by Wire::VALID_LAYOUT when a record is decoded or encoded.
namespace Wire {
template <>
struct Layout<ubx_payload_rx_nav_posllh_t>
{
    using T = ubx_payload_rx_nav_posllh_t;
    static constexpr size_t SIZE = 28;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::iTOW, 0>{},  Field<&T::lon, 4>{},   Field<&T::lat, 8>{},  Field<&T::height, 12>{},
                   Field<&T::hMSL, 16>{}, Field<&T::hAcc, 20>{}, Field<&T::vAcc, 24>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_dop_t>
{
    using T = ubx_payload_rx_nav_dop_t;
    static constexpr size_t SIZE = 18;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::iTOW, 0>{},  Field<&T::gDOP, 4>{},  Field<&T::pDOP, 6>{},  Field<&T::tDOP, 8>{},
                   Field<&T::vDOP, 10>{}, Field<&T::hDOP, 12>{}, Field<&T::nDOP, 14>{}, Field<&T::eDOP, 16>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_sol_t>
{
    using T = ubx_payload_rx_nav_sol_t;
    static constexpr size_t SIZE = 52;
    static constexpr auto FIELDS = std::tuple{
        Field<&T::iTOW, 0>{},      Field<&T::fTOW, 4>{},    Field<&T::week, 8>{},       Field<&T::gpsFix, 10>{},
        Field<&T::flags, 11>{},    Field<&T::ecefX, 12>{},  Field<&T::ecefY, 16>{},     Field<&T::ecefZ, 20>{},
        Field<&T::pAcc, 24>{},     Field<&T::ecefVX, 28>{}, Field<&T::ecefVY, 32>{},    Field<&T::ecefVZ, 36>{},
        Field<&T::sAcc, 40>{},     Field<&T::pDOP, 44>{},   Field<&T::reserved1, 46>{}, Field<&T::numSV, 47>{},
        Field<&T::reserved2, 48>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_pvt_t>
{
    using T = ubx_payload_rx_nav_pvt_t;
    static constexpr size_t SIZE = 92;
    static constexpr auto FIELDS = std::tuple{
        Field<&T::iTOW, 0>{},       Field<&T::year, 4>{},    Field<&T::month, 6>{},    Field<&T::day, 7>{},
        Field<&T::hour, 8>{},       Field<&T::min, 9>{},     Field<&T::sec, 10>{},     Field<&T::valid, 11>{},
        Field<&T::tAcc, 12>{},      Field<&T::nano, 16>{},   Field<&T::fixType, 20>{}, Field<&T::flags, 21>{},
        Field<&T::reserved1, 22>{}, Field<&T::numSV, 23>{},  Field<&T::lon, 24>{},     Field<&T::lat, 28>{},
        Field<&T::height, 32>{},    Field<&T::hMSL, 36>{},   Field<&T::hAcc, 40>{},    Field<&T::vAcc, 44>{},
        Field<&T::gSpeed, 60>{},    Field<&T::headMot, 64>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_timeutc_t>
{
    using T = ubx_payload_rx_nav_timeutc_t;
    static constexpr size_t SIZE = 20;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::iTOW, 0>{},   Field<&T::tAcc, 4>{},  Field<&T::nano, 8>{},  Field<&T::year, 12>{},
                   Field<&T::month, 14>{}, Field<&T::day, 15>{},  Field<&T::hour, 16>{}, Field<&T::min, 17>{},
                   Field<&T::sec, 18>{},   Field<&T::valid, 19>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_svinfo_part1_t>
{
    using T = ubx_payload_rx_nav_svinfo_part1_t;
    static constexpr size_t SIZE = 8;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::iTOW, 0>{}, Field<&T::numCh, 4>{}, Field<&T::globalFlags, 5>{}, Field<&T::reserved2, 6>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_svinfo_part2_t>
{
    using T = ubx_payload_rx_nav_svinfo_part2_t;
    static constexpr size_t SIZE = 12;
    static constexpr auto FIELDS = std::tuple{Field<&T::flags, 2>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_sat_part1_t>
{
    using T = ubx_payload_rx_nav_sat_part1_t;
    static constexpr size_t SIZE = 8;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::iTOW, 0>{}, Field<&T::version, 4>{}, Field<&T::numSvs, 5>{}, Field<&T::reserved, 6>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_sat_part2_t>
{
    using T = ubx_payload_rx_nav_sat_part2_t;
    static constexpr size_t SIZE = 12;
    static constexpr auto FIELDS = std::tuple{Field<&T::gnssId, 0>{}, Field<&T::flags, 8>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_status_t>
{
    using T = ubx_payload_rx_nav_status_t;
    static constexpr size_t SIZE = 16;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::iTOW, 0>{},   Field<&T::gpsFix, 4>{}, Field<&T::flags, 5>{}, Field<&T::fixStat, 6>{},
                   Field<&T::flags2, 7>{}, Field<&T::ttff, 8>{},   Field<&T::msss, 12>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_svin_t>
{
    using T = ubx_payload_rx_nav_svin_t;
    static constexpr size_t SIZE = 40;
    static constexpr auto FIELDS = std::tuple{
        Field<&T::version, 0>{},  Field<&T::reserved1, 1>{}, Field<&T::iTOW, 4>{},       Field<&T::dur, 8>{},
        Field<&T::meanX, 12>{},   Field<&T::meanY, 16>{},    Field<&T::meanZ, 20>{},     Field<&T::meanXHP, 24>{},
        Field<&T::meanYHP, 25>{}, Field<&T::meanZHP, 26>{},  Field<&T::reserved2, 27>{}, Field<&T::meanAcc, 28>{},
        Field<&T::obs, 32>{},     Field<&T::valid, 36>{},    Field<&T::active, 37>{},    Field<&T::reserved3, 38>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_velned_t>
{
    using T = ubx_payload_rx_nav_velned_t;
    static constexpr size_t SIZE = 36;
    static constexpr auto FIELDS = std::tuple{Field<&T::iTOW, 0>{}, Field<&T::gSpeed, 20>{}, Field<&T::heading, 24>{}};
};

template <>
struct Layout<ubx_payload_rx_mon_hw_ubx6_t>
{
    using T = ubx_payload_rx_mon_hw_ubx6_t;
    static constexpr size_t SIZE = 68;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::noisePerMS, 16>{}, Field<&T::agcCnt, 18>{}, Field<&T::jamInd, 53>{}};
};

template <>
struct Layout<ubx_payload_rx_mon_hw_ubx7_t>
{
    using T = ubx_payload_rx_mon_hw_ubx7_t;
    static constexpr size_t SIZE = 60;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::noisePerMS, 16>{}, Field<&T::agcCnt, 18>{}, Field<&T::jamInd, 45>{}};
};

template <>
struct Layout<ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t>
{
    using T = ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t;
    static constexpr size_t SIZE = 24;
    static constexpr auto FIELDS = std::tuple{
        Field<&T::blockId, 0>{},    Field<&T::flags, 1>{},     Field<&T::antStatus, 2>{},   Field<&T::antPower, 3>{},
        Field<&T::postStatus, 4>{}, Field<&T::reserved2, 8>{}, Field<&T::noisePerMS, 12>{}, Field<&T::agcCnt, 14>{},
        Field<&T::jamInd, 16>{},    Field<&T::ofsI, 17>{},     Field<&T::magI, 18>{},       Field<&T::ofsQ, 19>{},
        Field<&T::magQ, 20>{},      Field<&T::reserved3, 21>{}};
};

template <>
struct Layout<ubx_payload_rx_mon_rf_t>
{
    using T = ubx_payload_rx_mon_rf_t;
    static constexpr size_t SIZE = 28;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::version, 0>{}, Field<&T::nBlocks, 1>{}, Field<&T::reserved1, 2>{}, Field<&T::block, 4>{}};
};

template <>
struct Layout<ubx_payload_rx_sec_sig_t>
{
    using T = ubx_payload_rx_sec_sig_t;
    static constexpr size_t SIZE = 5;
    static constexpr auto FIELDS = std::tuple{Field<&T::version, 0>{}, Field<&T::flags, 1>{}, Field<&T::reserved0, 2>{},
                                              Field<&T::jamNumCentFreqs, 3>{}, Field<&T::jamFlags, 4>{}};
};

template <>
struct Layout<ubx_payload_rx_mon_ver_part1_t>
{
    using T = ubx_payload_rx_mon_ver_part1_t;
    static constexpr size_t SIZE = 40;
    static constexpr auto FIELDS = std::tuple{Field<&T::swVersion, 0>{}, Field<&T::hwVersion, 30>{}};
};

template <>
struct Layout<ubx_payload_rx_mon_ver_part2_t>
{
    using T = ubx_payload_rx_mon_ver_part2_t;
    static constexpr size_t SIZE = 30;
    static constexpr auto FIELDS = std::tuple{Field<&T::extension, 0>{}};
};

template <>
struct Layout<ubx_payload_rx_rxm_rtcm_t>
{
    using T = ubx_payload_rx_rxm_rtcm_t;
    static constexpr size_t SIZE = 8;
    static constexpr auto FIELDS = std::tuple{Field<&T::version, 0>{}, Field<&T::flags, 1>{}, Field<&T::subType, 2>{},
                                              Field<&T::refStationID, 4>{}, Field<&T::msgType, 6>{}};
};

template <>
struct Layout<ubx_payload_rx_rxm_cor_t>
{
    using T = ubx_payload_rx_rxm_cor_t;
    static constexpr size_t SIZE = 12;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::version, 0>{},    Field<&T::ebno, 1>{},    Field<&T::reserved0, 2>{},
                   Field<&T::statusInfo, 4>{}, Field<&T::msgType, 8>{}, Field<&T::msgSubType, 10>{}};
};

template <>
struct Layout<ubx_payload_rx_ack_ack_t>
{
    using T = ubx_payload_rx_ack_ack_t;
    static constexpr size_t SIZE = 2;
    static constexpr auto FIELDS = std::tuple{Field<&T::msg, 0>{}};
};

template <>
struct Layout<ubx_payload_tx_cfg_prt_t>
{
    using T = ubx_payload_tx_cfg_prt_t;
    static constexpr size_t SIZE = 20;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::portID, 0>{},        Field<&T::reserved0, 1>{}, Field<&T::txReady, 2>{},
                   Field<&T::mode, 4>{},          Field<&T::baudRate, 8>{},  Field<&T::inProtoMask, 12>{},
                   Field<&T::outProtoMask, 14>{}, Field<&T::flags, 16>{},    Field<&T::reserved5, 18>{}};
};

template <>
struct Layout<ubx_payload_tx_cfg_rate_t>
{
    using T = ubx_payload_tx_cfg_rate_t;
    static constexpr size_t SIZE = 6;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::measRate, 0>{}, Field<&T::navRate, 2>{}, Field<&T::timeRef, 4>{}};
};

template <>
struct Layout<ubx_payload_tx_cfg_nav5_t>
{
    using T = ubx_payload_tx_cfg_nav5_t;
    static constexpr size_t SIZE = 36;
    static constexpr auto FIELDS = std::tuple{Field<&T::mask, 0>{},
                                              Field<&T::dynModel, 2>{},
                                              Field<&T::fixMode, 3>{},
                                              Field<&T::fixedAlt, 4>{},
                                              Field<&T::fixedAltVar, 8>{},
                                              Field<&T::minElev, 12>{},
                                              Field<&T::drLimit, 13>{},
                                              Field<&T::pDop, 14>{},
                                              Field<&T::tDop, 16>{},
                                              Field<&T::pAcc, 18>{},
                                              Field<&T::tAcc, 20>{},
                                              Field<&T::staticHoldThresh, 22>{},
                                              Field<&T::dgpsTimeOut, 23>{},
                                              Field<&T::cnoThreshNumSVs, 24>{},
                                              Field<&T::cnoThresh, 25>{},
                                              Field<&T::reserved, 26>{},
                                              Field<&T::staticHoldMaxDist, 28>{},
                                              Field<&T::utcStandard, 30>{},
                                              Field<&T::reserved3, 31>{},
                                              Field<&T::reserved4, 32>{}};
};

template <>
struct Layout<ubx_payload_tx_cfg_msg_t>
{
    using T = ubx_payload_tx_cfg_msg_t;
    static constexpr size_t SIZE = 3;
    static constexpr auto FIELDS = std::tuple{Field<&T::msg, 0>{}, Field<&T::rate, 2>{}};
};

template <>
struct Layout<ubx_payload_tx_cfg_tmode3_t>
{
    using T = ubx_payload_tx_cfg_tmode3_t;
    static constexpr size_t SIZE = 40;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::version, 0>{},       Field<&T::reserved1, 1>{},     Field<&T::flags, 2>{},
                   Field<&T::ecefXOrLat, 4>{},    Field<&T::ecefYOrLon, 8>{},    Field<&T::ecefZOrAlt, 12>{},
                   Field<&T::ecefXOrLatHP, 16>{}, Field<&T::ecefYOrLonHP, 17>{}, Field<&T::ecefZOrAltHP, 18>{},
                   Field<&T::reserved2, 19>{},    Field<&T::fixedPosAcc, 20>{},  Field<&T::svinMinDur, 24>{},
                   Field<&T::svinAccLimit, 28>{}, Field<&T::reserved3, 32>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_relposned_t>
{
    using T = ubx_payload_rx_nav_relposned_t;
    static constexpr size_t SIZE = 64;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::version, 0>{},       Field<&T::reserved0, 1>{},      Field<&T::iTOW, 4>{},
                   Field<&T::relPosLength, 20>{}, Field<&T::relPosHeading, 24>{}, Field<&T::relPosHPLength, 35>{},
                   Field<&T::accHeading, 52>{},   Field<&T::flags, 60>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_daheading_t>
{
    using T = ubx_payload_rx_nav_daheading_t;
    static constexpr size_t SIZE = 60;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::version, 0>{},       Field<&T::reserved0, 1>{},      Field<&T::iTOW, 4>{},
                   Field<&T::relPosLength, 20>{}, Field<&T::relPosHeading, 24>{}, Field<&T::accHeading, 48>{},
                   Field<&T::flags, 56>{}};
};

template <>
struct Layout<ubx_payload_rx_nav_hpposllh_t>
{
    using T = ubx_payload_rx_nav_hpposllh_t;
    static constexpr size_t SIZE = 36;
    static constexpr auto FIELDS = std::tuple{
        Field<&T::version, 0>{}, Field<&T::reserved1, 1>{}, Field<&T::flags, 3>{},     Field<&T::iTOW, 4>{},
        Field<&T::lon, 8>{},     Field<&T::lat, 12>{},      Field<&T::height, 16>{},   Field<&T::hMSL, 20>{},
        Field<&T::lonHp, 24>{},  Field<&T::latHp, 25>{},    Field<&T::heightHp, 26>{}, Field<&T::hMSLHp, 27>{},
        Field<&T::hAcc, 28>{},   Field<&T::vAcc, 32>{}};
};

template <>
struct Layout<ubx_payload_rx_mon_comms_port_t>
{
    using T = ubx_payload_rx_mon_comms_port_t;
    static constexpr size_t SIZE = 40;
    static constexpr auto FIELDS = std::tuple{
        Field<&T::portId, 0>{},       Field<&T::txPending, 2>{},    Field<&T::txBytes, 4>{},  Field<&T::txUsage, 8>{},
        Field<&T::txPeakUsage, 9>{},  Field<&T::rxPending, 10>{},   Field<&T::rxBytes, 12>{}, Field<&T::rxUsage, 16>{},
        Field<&T::rxPeakUsage, 17>{}, Field<&T::overrunErrs, 18>{}, Field<&T::msgs, 20>{},    Field<&T::reserved, 28>{},
        Field<&T::skipped, 36>{}};
};

template <>
struct Layout<ubx_payload_rx_mon_comms_t>
{
    using T = ubx_payload_rx_mon_comms_t;
    static constexpr size_t SIZE = 328;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::version, 0>{},  Field<&T::nPorts, 1>{},  Field<&T::txErrors, 2>{},
                   Field<&T::reserved, 3>{}, Field<&T::protIds, 4>{}, Field<&T::ports, 8>{}};
};

}  // namespace Wire

namespace UBX {
template <Wire::Record T>
inline constexpr size_t WIRE_SIZE = Wire::SIZE<T>;

inline constexpr size_t MAX_CONTROL_PAYLOAD_SIZE = 328;
inline constexpr size_t MON_HW_DEPRECATED_SIZE = 56;
inline constexpr uint16_t NAV_EOE = 0x6101;
inline constexpr uint32_t NAV_EOE_MSGOUT_I2C = 0x2091015f;
inline constexpr double DEGREES_PER_COORDINATE = 1e-7;

/// Converts a 1e-7 degree coordinate; values beyond +/-@a limitDegrees come from a corrupt or
/// uninitialised solution and are reported as unavailable (NaN).
[[nodiscard]] inline double coordinateDegrees(int32_t value, int64_t limitDegrees)
{
    return std::llabs(value) <= limitDegrees * 10'000'000 ? value * DEGREES_PER_COORDINATE
                                                          : std::numeric_limits<double>::quiet_NaN();
}

[[nodiscard]] inline double latitudeDegrees(int32_t value)
{
    return coordinateDegrees(value, 90);
}

[[nodiscard]] inline double longitudeDegrees(int32_t value)
{
    return coordinateDegrees(value, 180);
}

inline constexpr float DOP_PER_UNIT = 0.01f;

struct MessageSchema
{
    uint16_t message;
    size_t minimum;
    size_t maximum;
    size_t stride;
    int towOffset;
};

template <typename T>
[[nodiscard]] constexpr MessageSchema fixedSchema(uint16_t message, int towOffset = -1)
{
    return {message, WIRE_SIZE<T>, WIRE_SIZE<T>, 1, towOffset};
}

template <typename Header, typename Block>
[[nodiscard]] constexpr MessageSchema repeatedSchema(uint16_t message, size_t maximumBlocks)
{
    return {message, WIRE_SIZE<Header>, WIRE_SIZE<Header> + WIRE_SIZE<Block> * maximumBlocks, WIRE_SIZE<Block>, -1};
}

inline constexpr std::array MESSAGE_SCHEMAS = {
    MessageSchema{UBX_MSG_NAV_PVT, 84, WIRE_SIZE<ubx_payload_rx_nav_pvt_t>, 8, 0},
    fixedSchema<ubx_payload_rx_nav_posllh_t>(UBX_MSG_NAV_POSLLH, 0),
    fixedSchema<ubx_payload_rx_nav_hpposllh_t>(UBX_MSG_NAV_HPPOSLLH, 4),
    fixedSchema<ubx_payload_rx_nav_sol_t>(UBX_MSG_NAV_SOL, 0),
    fixedSchema<ubx_payload_rx_nav_status_t>(UBX_MSG_NAV_STATUS),
    fixedSchema<ubx_payload_rx_nav_dop_t>(UBX_MSG_NAV_DOP, 0),
    fixedSchema<ubx_payload_rx_nav_relposned_t>(UBX_MSG_NAV_RELPOSNED, 4),
    fixedSchema<ubx_payload_rx_nav_daheading_t>(UBX_MSG_NAV_DAHEADING, 4),
    fixedSchema<ubx_payload_rx_nav_timeutc_t>(UBX_MSG_NAV_TIMEUTC, 0),
    fixedSchema<ubx_payload_rx_nav_velned_t>(UBX_MSG_NAV_VELNED, 0),
    fixedSchema<ubx_payload_rx_nav_svin_t>(UBX_MSG_NAV_SVIN),
    MessageSchema{NAV_EOE, 4, 4, 1, 0},
    repeatedSchema<ubx_payload_rx_nav_sat_part1_t, ubx_payload_rx_nav_sat_part2_t>(UBX_MSG_NAV_SAT, 255),
    repeatedSchema<ubx_payload_rx_nav_svinfo_part1_t, ubx_payload_rx_nav_svinfo_part2_t>(UBX_MSG_NAV_SVINFO, 255),
    MessageSchema{UBX_MSG_MON_VER, WIRE_SIZE<ubx_payload_rx_mon_ver_part1_t>, 4090,
                  WIRE_SIZE<ubx_payload_rx_mon_ver_part2_t>, -1},
    MessageSchema{UBX_MSG_MON_HW, MON_HW_DEPRECATED_SIZE, WIRE_SIZE<ubx_payload_rx_mon_hw_ubx6_t>, 4, -1},
    // The decoded record holds the first of up to 255 RF blocks.
    repeatedSchema<ubx_payload_rx_mon_rf_t, ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t>(UBX_MSG_MON_RF,
                                                                                                    254),
    MessageSchema{UBX_MSG_MON_COMMS, 8, WIRE_SIZE<ubx_payload_rx_mon_comms_t>,
                  WIRE_SIZE<ubx_payload_rx_mon_comms_port_t>, -1},
    MessageSchema{UBX_MSG_SEC_SIG, 4, 4096, 1, -1},
    fixedSchema<ubx_payload_rx_rxm_rtcm_t>(UBX_MSG_RXM_RTCM),
    fixedSchema<ubx_payload_rx_rxm_cor_t>(UBX_MSG_RXM_COR),
    fixedSchema<ubx_payload_rx_ack_ack_t>(UBX_MSG_ACK_ACK),
    fixedSchema<ubx_payload_rx_ack_ack_t>(UBX_MSG_ACK_NAK),
    MessageSchema{UBX_MSG_CFG_VALGET, 4, MAX_CONTROL_PAYLOAD_SIZE, 1, -1},
    fixedSchema<ubx_payload_tx_cfg_tmode3_t>(UBX_MSG_CFG_TMODE3),
};

[[nodiscard]] constexpr const MessageSchema* messageSchema(uint16_t message)
{
    for (const auto& schema : MESSAGE_SCHEMAS) {
        if (schema.message == message) {
            return &schema;
        }
    }
    return nullptr;
}

/// @param schema messageSchema(message), passed by callers that also need the schema.
[[nodiscard]] inline bool validPayload(uint16_t message, std::span<const uint8_t> payload, const MessageSchema* schema)
{
    if (!schema) {
        return true;
    }
    if (payload.size() < schema->minimum || payload.size() > schema->maximum ||
        (payload.size() - schema->minimum) % schema->stride != 0) {
        return false;
    }
    if (message == UBX_MSG_NAV_SAT || message == UBX_MSG_NAV_SVINFO) {
        return payload.size() ==
                   schema->minimum + schema->stride * size_t(payload[message == UBX_MSG_NAV_SAT ? 5 : 4]) &&
               (message != UBX_MSG_NAV_SAT || payload[4] == 1);
    }
    if (message == UBX_MSG_MON_HW) {
        return payload.size() == MON_HW_DEPRECATED_SIZE || payload.size() == WIRE_SIZE<ubx_payload_rx_mon_hw_ubx7_t> ||
               payload.size() == WIRE_SIZE<ubx_payload_rx_mon_hw_ubx6_t>;
    }
    if (message == UBX_MSG_MON_RF) {
        return payload[0] == 0 && payload[1] > 0 &&
               payload.size() == schema->minimum + schema->stride * (size_t(payload[1]) - 1);
    }
    if (message == UBX_MSG_MON_COMMS) {
        return payload[0] == 0 && payload.size() == schema->minimum + schema->stride * size_t(payload[1]);
    }
    if (message == UBX_MSG_SEC_SIG) {
        return payload[0] == 1 ? payload.size() >= 5
                               : (payload[0] == 2 || payload[0] == 3) && payload.size() == 4 + 4 * size_t(payload[3]);
    }
    if (message == UBX_MSG_RXM_COR) {
        return payload[0] == 1;
    }
    return true;
}

[[nodiscard]] inline bool validPayload(uint16_t message, std::span<const uint8_t> payload)
{
    return validPayload(message, payload, messageSchema(message));
}
}  // namespace UBX
