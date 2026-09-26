#pragma once

#include <cstdint>

#include "MAVLinkLib.h"

namespace GPSTestHelpers {

/// GPS_RAW_INT and GPS2_RAW fields; unset fields are zero, as in a default-initialized message.
struct GPSRawFields
{
    int32_t latitudeE7 = 0;
    int32_t longitudeE7 = 0;
    int32_t altitudeMm = 0;
    uint8_t fixType = GPS_FIX_TYPE_NO_FIX;
    uint16_t eph = 0;
    uint16_t epv = 0;
    uint16_t cog = 0;
    uint8_t satellitesVisible = 0;
    uint16_t yaw = 0;
    uint32_t horizontalAccuracyMm = 0;
    uint32_t verticalAccuracyMm = 0;
    uint64_t timeUsec = 0;
};

enum class GPSReceiverIndex
{
    Primary,
    Secondary,
};

inline mavlink_message_t gpsRawMessage(const GPSRawFields& fields,
                                       GPSReceiverIndex receiver = GPSReceiverIndex::Primary, uint8_t systemId = 1,
                                       uint8_t componentId = 1)
{
    const auto fill = [&fields](auto& raw) {
        raw.time_usec = fields.timeUsec;
        raw.lat = fields.latitudeE7;
        raw.lon = fields.longitudeE7;
        raw.alt = fields.altitudeMm;
        raw.fix_type = fields.fixType;
        raw.eph = fields.eph;
        raw.epv = fields.epv;
        raw.cog = fields.cog;
        raw.satellites_visible = fields.satellitesVisible;
        raw.yaw = fields.yaw;
        raw.h_acc = fields.horizontalAccuracyMm;
        raw.v_acc = fields.verticalAccuracyMm;
    };
    mavlink_message_t message{};
    if (receiver == GPSReceiverIndex::Secondary) {
        mavlink_gps2_raw_t raw{};
        fill(raw);
        mavlink_msg_gps2_raw_encode(systemId, componentId, &message, &raw);
    } else {
        mavlink_gps_raw_int_t raw{};
        fill(raw);
        mavlink_msg_gps_raw_int_encode(systemId, componentId, &message, &raw);
    }
    return message;
}

}  // namespace GPSTestHelpers
