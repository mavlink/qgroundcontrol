#include "GPSTestHelpers.h"

#include "MAVLinkLib.h"

namespace GPSTest {

mavlink_message_t gpsRawMessage(const GPSRawFields& fields, GPSReceiverIndex receiver, uint8_t systemId,
                                uint8_t componentId)
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

}  // namespace GPSTest
