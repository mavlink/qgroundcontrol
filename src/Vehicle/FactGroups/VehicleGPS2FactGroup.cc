#include "VehicleGPS2FactGroup.h"
#include "Vehicle.h"
#include "development/mavlink_msg_gnss_integrity.h"

void VehicleGPS2FactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_GPS2_RAW:
        _handleGps2Raw(message);
        break;
    case MAVLINK_MSG_ID_GNSS_INTEGRITY:
        _handleGnssIntegrity(message);
        break;
    case MAVLINK_MSG_ID_GPS2_RTK:
        _handleGpsRtk(message);
        break;
    default:
        break;
    }
}

void VehicleGPS2FactGroup::_handleGps2Raw(const mavlink_message_t &message)
{
    mavlink_gps2_raw_t gps2Raw{};
    mavlink_msg_gps2_raw_decode(&message, &gps2Raw);
    _applyGpsRawFields(gps2Raw);
}
