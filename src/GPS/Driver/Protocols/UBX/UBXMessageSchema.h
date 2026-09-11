#pragma once

#include <array>
#include <span>

#include "UBXMessages.h"

namespace UBX {
inline constexpr size_t MAX_CONTROL_PAYLOAD_SIZE = 328;
inline constexpr size_t MON_HW_DEPRECATED_SIZE = 56;
inline constexpr uint16_t NAV_EOE = 0x6101;
inline constexpr uint32_t NAV_EOE_MSGOUT_I2C = 0x2091015f;
inline constexpr double DEGREES_PER_COORDINATE = 1e-7;
inline constexpr float DOP_PER_UNIT = 0.01f;

struct MessageSchema
{
    uint16_t message;
    size_t minimum;
    size_t maximum;
    size_t stride;
    int towOffset;
};

inline constexpr std::array MESSAGE_SCHEMAS = {
    MessageSchema{UBX_MSG_NAV_PVT, 84, 92, 8, 0},
    MessageSchema{UBX_MSG_NAV_POSLLH, 28, 28, 1, 0},
    MessageSchema{UBX_MSG_NAV_HPPOSLLH, 36, 36, 1, 4},
    MessageSchema{UBX_MSG_NAV_SOL, 52, 52, 1, 0},
    MessageSchema{UBX_MSG_NAV_STATUS, 16, 16, 1, -1},
    MessageSchema{UBX_MSG_NAV_DOP, 18, 18, 1, 0},
    MessageSchema{UBX_MSG_NAV_RELPOSNED, 64, 64, 1, 4},
    MessageSchema{UBX_MSG_NAV_DAHEADING, UBX::WIRE_SIZE<ubx_payload_rx_nav_daheading_t>,
                  UBX::WIRE_SIZE<ubx_payload_rx_nav_daheading_t>, 1, 4},
    MessageSchema{UBX_MSG_NAV_TIMEUTC, 20, 20, 1, 0},
    MessageSchema{UBX_MSG_NAV_VELNED, 36, 36, 1, 0},
    MessageSchema{UBX_MSG_NAV_SVIN, 40, 40, 1, -1},
    MessageSchema{NAV_EOE, 4, 4, 1, 0},
    MessageSchema{UBX_MSG_NAV_SAT, 8, 8 + 12 * 255, 12, -1},
    MessageSchema{UBX_MSG_NAV_SVINFO, 8, 8 + 12 * 255, 12, -1},
    MessageSchema{UBX_MSG_MON_VER, 40, 4090, 30, -1},
    MessageSchema{UBX_MSG_MON_HW, 56, 68, 4, -1},
    MessageSchema{UBX_MSG_MON_RF, 28, 4 + 24 * 255, 24, -1},
    MessageSchema{UBX_MSG_MON_COMMS, 8, MAX_CONTROL_PAYLOAD_SIZE, 40, -1},
    MessageSchema{UBX_MSG_SEC_SIG, 4, 4096, 1, -1},
    MessageSchema{UBX_MSG_RXM_RTCM, 8, 8, 1, -1},
    MessageSchema{UBX_MSG_RXM_COR, 12, 12, 1, -1},
    MessageSchema{UBX_MSG_ACK_ACK, 2, 2, 1, -1},
    MessageSchema{UBX_MSG_ACK_NAK, 2, 2, 1, -1},
    MessageSchema{UBX_MSG_CFG_VALGET, 4, MAX_CONTROL_PAYLOAD_SIZE, 1, -1},

};

constexpr const MessageSchema* messageSchema(uint16_t message)
{
    for (const auto& schema : MESSAGE_SCHEMAS)
        if (schema.message == message)
            return &schema;
    return nullptr;
}

inline bool validPayload(uint16_t message, std::span<const uint8_t> payload)
{
    const auto* schema = messageSchema(message);
    if (!schema)
        return true;
    if (payload.size() < schema->minimum || payload.size() > schema->maximum ||
        (payload.size() - schema->minimum) % schema->stride != 0)
        return false;
    if (message == UBX_MSG_NAV_SAT || message == UBX_MSG_NAV_SVINFO)
        return payload.size() == 8 + 12 * size_t(payload[message == UBX_MSG_NAV_SAT ? 5 : 4]) &&
               (message != UBX_MSG_NAV_SAT || payload[4] == 1);
    if (message == UBX_MSG_MON_HW)
        return payload.size() == 56 || payload.size() == 60 || payload.size() == 68;
    if (message == UBX_MSG_MON_RF)
        return payload[0] == 0 && payload[1] > 0 && payload.size() == 4 + 24 * size_t(payload[1]);
    if (message == UBX_MSG_MON_COMMS)
        return payload[0] == 0 && payload.size() == 8 + 40 * size_t(payload[1]);
    if (message == UBX_MSG_SEC_SIG)
        return payload[0] == 1 ? payload.size() >= 5
                               : (payload[0] == 2 || payload[0] == 3) && payload.size() == 4 + 4 * size_t(payload[3]);
    if (message == UBX_MSG_RXM_COR)
        return payload[0] == 1;
    return true;
}

static_assert(UBX::WIRE_SIZE<ubx_payload_rx_nav_pvt_t> == 92);
static_assert(UBX::WIRE_SIZE<ubx_payload_rx_nav_hpposllh_t> == 36);
static_assert(UBX::WIRE_SIZE<ubx_payload_rx_nav_relposned_t> == 64);
}  // namespace UBX
