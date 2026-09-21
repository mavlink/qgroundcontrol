#include "VehicleGPS2FactGroup.h"

#include "MAVLinkLib.h"

void VehicleGPS2FactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_GPS2_RAW:
        _handleGpsRaw(message);
        break;
    case MAVLINK_MSG_ID_GNSS_INTEGRITY:
        _handleGnssIntegrity(message);
        break;
    default:
        break;
    }
}
