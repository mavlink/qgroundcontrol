#pragma once

#include "MAVLinkMessage.h"
#include <MAVLinkLib.h>
#include <QtCore/QObject>

namespace MAVLinkTestHelpers {

// Build a raw mavlink heartbeat message
inline mavlink_message_t makeHeartbeat(uint8_t sysId = 1, uint8_t compId = 1)
{
    mavlink_message_t msg{};
    mavlink_msg_heartbeat_pack_chan(
        sysId, compId, MAVLINK_COMM_0, &msg,
        MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_PX4, 0, 0, MAV_STATE_ACTIVE);
    return msg;
}

// Build a QGCMAVLinkMessage wrapping a heartbeat
inline QGCMAVLinkMessage* makeHeartbeatMsg(uint8_t sysId, uint8_t compId, QObject* parent = nullptr)
{
    mavlink_message_t msg = makeHeartbeat(sysId, compId);
    return new QGCMAVLinkMessage(msg, parent);
}

inline QGCMAVLinkMessage* makeHeartbeatMsg(QObject* parent = nullptr)
{
    return makeHeartbeatMsg(1, 1, parent);
}

} // namespace MAVLinkTestHelpers
