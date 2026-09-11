#pragma once

#include <optional>
#include <span>
#include <type_traits>

#include "UBXMessageSchema.h"
#include "UBXWire.h"

namespace UBX {
template <typename T>
inline constexpr uint16_t MESSAGE_ID = 0;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_posllh_t> = UBX_MSG_NAV_POSLLH;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_dop_t> = UBX_MSG_NAV_DOP;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_sol_t> = UBX_MSG_NAV_SOL;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_pvt_t> = UBX_MSG_NAV_PVT;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_timeutc_t> = UBX_MSG_NAV_TIMEUTC;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_status_t> = UBX_MSG_NAV_STATUS;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_svin_t> = UBX_MSG_NAV_SVIN;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_velned_t> = UBX_MSG_NAV_VELNED;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_mon_hw_ubx6_t> = UBX_MSG_MON_HW;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_mon_hw_ubx7_t> = UBX_MSG_MON_HW;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_sec_sig_t> = UBX_MSG_SEC_SIG;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_rxm_rtcm_t> = UBX_MSG_RXM_RTCM;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_rxm_cor_t> = UBX_MSG_RXM_COR;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_relposned_t> = UBX_MSG_NAV_RELPOSNED;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_daheading_t> = UBX_MSG_NAV_DAHEADING;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_nav_hpposllh_t> = UBX_MSG_NAV_HPPOSLLH;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_mon_comms_t> = UBX_MSG_MON_COMMS;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_mon_rf_t> = UBX_MSG_MON_RF;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_ack_ack_t> = UBX_MSG_ACK_ACK;
template <>
inline constexpr uint16_t MESSAGE_ID<ubx_payload_rx_ack_nak_t> = UBX_MSG_ACK_NAK;

template <typename T>
struct MessageCodec
{
    static constexpr uint16_t MESSAGE = MESSAGE_ID<T>;

    static std::optional<T> decode(std::span<const uint8_t> bytes)
    {
        if constexpr (MESSAGE != 0) {
            if (!validPayload(MESSAGE, bytes))
                return std::nullopt;
            if constexpr (std::is_same_v<T, ubx_payload_rx_mon_hw_ubx6_t> ||
                          std::is_same_v<T, ubx_payload_rx_mon_hw_ubx7_t>) {
                if (bytes.size() != WIRE_SIZE<T>)
                    return std::nullopt;
            }
        } else if (bytes.size() != WIRE_SIZE<T>) {
            return std::nullopt;
        }
        return detail::decodeFields<T>(bytes);
    }

    static std::optional<T> block(std::span<const uint8_t> bytes, size_t offset = 0)
    {
        if (offset > bytes.size() || WIRE_SIZE<T> > bytes.size() - offset)
            return std::nullopt;
        return decode(bytes.subspan(offset, WIRE_SIZE<T>));
    }
};
}  // namespace UBX
