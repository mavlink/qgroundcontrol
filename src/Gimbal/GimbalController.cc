#include "GimbalController.h"

QGC_LOGGING_CATEGORY(GimbalLog, "GimbalLog")


GimbalController::GimbalController(Vehicle *vehicle)
    : _vehicle(vehicle)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &GimbalController::_mavlinkMessageReceived);
}

void
GimbalController::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT:
            _handleHeartbeat(message);
            break;
        case MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION:
            _handleGimbalManagerInformation(message);
            break;
        case MAVLINK_MSG_ID_GIMBAL_MANAGER_STATUS:
            _handleGimbalManagerStatus(message);
            break;
        case MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS:
            _handleGimbalDeviceAttitudeStatus(message);
            break;
    }
}

void    
GimbalController::_handleHeartbeat(const mavlink_message_t& message)
{
    if (!_potentialGimbals.contains(message.compid)) {
        qCDebug(GimbalLog) << "new potential gimbal component: " << message.compid;
    }

    auto& gimbal = _potentialGimbals[message.compid];

    if (!gimbal.receivedInformation && gimbal.requestInformationRetries > 0) {
        _requestGimbalInformation(message.compid);
        --gimbal.requestInformationRetries;
    }

    if (!gimbal.receivedStatus && gimbal.requestStatusRetries > 0) {
        _vehicle->sendMavCommand(message.compid,
                                 MAV_CMD_SET_MESSAGE_INTERVAL,
                                 false /* no error */,
                                 MAVLINK_MSG_ID_GIMBAL_MANAGER_STATUS,
                                 0 /* request default rate */);
        --gimbal.requestStatusRetries;
    }

    if (!gimbal.receivedAttitude && gimbal.requestAttitudeRetries > 0 &&
        gimbal.receivedInformation && gimbal.responsibleCompid != 0) {
        // We request the attitude directly from the gimbal device component.
        // We can only do that once we have received the gimbal manager information
        // telling us which gimbal device it is responsible for.
        _vehicle->sendMavCommand(gimbal.responsibleCompid,
                                 MAV_CMD_SET_MESSAGE_INTERVAL,
                                 false /* no error */,
                                 MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS,
                                 0 /* request default rate */);

        --gimbal.requestAttitudeRetries;
    }
}

void
GimbalController::_handleGimbalManagerInformation(const mavlink_message_t& message)
{
    qCDebug(GimbalLog) << "_handleGimbalManagerInformation for component" << message.compid;

    auto& gimbal = _potentialGimbals[message.compid];

    mavlink_gimbal_manager_information_t information;
    mavlink_msg_gimbal_manager_information_decode(&message, &information);

    gimbal.receivedInformation = true;
    gimbal.responsibleCompid = information.gimbal_device_id;
}

void
GimbalController::_handleGimbalManagerStatus(const mavlink_message_t& message)
{
    qCDebug(GimbalLog) << "_handleGimbalManagerStatus for component" << message.compid;

    auto& gimbal = _potentialGimbals[message.compid];

    gimbal.receivedStatus = true;
}

void
GimbalController::_handleGimbalDeviceAttitudeStatus(const mavlink_message_t& message)
{
    // We do a reverse lookup here because the gimbal device might have a
    // different component ID than the gimbal manager.
    for (auto& gimbal : _potentialGimbals) {
        if (gimbal.responsibleCompid == message.compid) {
            gimbal.receivedAttitude = true;
        }
    }
}

void
GimbalController::_requestGimbalInformation(uint8_t compid)
{
    qCDebug(GimbalLog) << "_requestGimbalInformation(" << compid << ")";

    if(_vehicle) {
        _vehicle->requestMessage(_requestMessageHandler, this,
                                 compid, MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION);
    }
}

void GimbalController::_requestMessageHandler(void* resultHandlerData,
                                              MAV_RESULT commandResult,
                                              Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                              const mavlink_message_t& message)
{
    // GimbalController* self = (GimbalController*)resultHandlerData;

    qCDebug(GimbalLog) << "_requestMessageHandler, commandResult: " << commandResult << ", failureCode: " << failureCode;

    // Not sure what to do here, we already subscribed to GimbalInformation in general in case it
    // arrives without us asking for it.
}
