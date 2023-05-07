#include "GimbalController.h"

#include "MAVLinkProtocol.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(GimbalLog, "GimbalLog")


GimbalController::GimbalController(MAVLinkProtocol* mavlink, Vehicle* vehicle)
    : _mavlink(mavlink)
    , _vehicle(vehicle)
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
    qCDebug(GimbalLog) << "_handleGimbalManagerInformation for component " << message.compid;

    auto& gimbal = _potentialGimbals[message.compid];

    mavlink_gimbal_manager_information_t information;
    mavlink_msg_gimbal_manager_information_decode(&message, &information);

    gimbal.receivedInformation = true;
    if (information.gimbal_device_id != 0) {
        gimbal.responsibleCompid = information.gimbal_device_id;
        qCDebug(GimbalLog) << "gimbal manager " << message.compid
            << " is responsible for gimbal device " << information.gimbal_device_id;
    }

    _checkComplete(gimbal);
}

void
GimbalController::_handleGimbalManagerStatus(const mavlink_message_t& message)
{
    qCDebug(GimbalLog) << "_handleGimbalManagerStatus for component " << message.compid;

    auto& gimbal = _potentialGimbals[message.compid];

    mavlink_gimbal_manager_status_t status;
    mavlink_msg_gimbal_manager_status_decode(&message, &status);

    gimbal.receivedStatus = true;

    if (status.gimbal_device_id != 0) {
        gimbal.responsibleCompid = status.gimbal_device_id;
        qCDebug(GimbalLog) << "gimbal manager " << message.compid
            << " is responsible for gimbal device " << status.gimbal_device_id;
    }

    const bool haveControl =
        (status.primary_control_sysid == _mavlink->getSystemId()) &&
        (status.primary_control_compid == _mavlink->getComponentId());

    const bool othersHaveControl = !haveControl &&
        (status.primary_control_sysid != 0 && status.primary_control_compid != 0); 

    bool hasChanged = false;

    if (gimbal.haveControl != haveControl) {
        gimbal.haveControl = haveControl;
        hasChanged = true;
    }

    if (gimbal.othersHaveControl != othersHaveControl) {
        gimbal.othersHaveControl = othersHaveControl;
        hasChanged = true;
    }

    if (hasChanged) {
        //emit gimbalsChanged();
    }

    _checkComplete(gimbal);
}

void
GimbalController::_handleGimbalDeviceAttitudeStatus(const mavlink_message_t& message)
{
    // We do a reverse lookup here because the gimbal device might have a
    // different component ID than the gimbal manager.
    auto gimbal_it = std::find_if(_potentialGimbals.begin(), _potentialGimbals.end(),
                 [&](auto& gimbal) { return gimbal.responsibleCompid == message.compid; });

    if (gimbal_it == _potentialGimbals.end()) {
        qCDebug(GimbalLog) << "_handleGimbalDeviceAttitudeStatus for unknown device with component " << message.compid;
    }

    mavlink_gimbal_device_attitude_status_t attitude_status;
    mavlink_msg_gimbal_device_attitude_status_decode(&message, &attitude_status);

    gimbal_it->retracted = (attitude_status.flags & GIMBAL_MANAGER_FLAGS_RETRACT) > 0;
    gimbal_it->neutral = (attitude_status.flags & GIMBAL_MANAGER_FLAGS_NEUTRAL) > 0;
    gimbal_it->yawLock = (attitude_status.flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK) > 0;

    // TODO: use more appropriate conversion
    float roll, pitch, yaw;
    mavlink_quaternion_to_euler(attitude_status.q, &roll, &pitch, &yaw);

    gimbal_it->curPitch = pitch * (180.0f / M_PI);
    gimbal_it->curRoll  = roll * (180.0f / M_PI);
    gimbal_it->curYaw   = yaw * (180.0f / M_PI);

    _checkComplete(*gimbal_it);
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

    qCDebug(GimbalLog) << "_requestMessageHandler, commandResult: " << commandResult << ", failureCode: " << failureCode << "message id: " << message.msgid;

    // Not sure what to do here, we already subscribed to GimbalInformation in general in case it
    // arrives without us asking for it.
}

void GimbalController::_checkComplete(Gimbal& gimbal)
{
    if (gimbal.isComplete) {
        // Already complete, nothing to do.
        return;
    }

    if (!gimbal.receivedInformation || !gimbal.receivedStatus || !gimbal.receivedAttitude) {
        // Not complete yet.
        return;
    }

    gimbal.isComplete = true;

    _gimbals.push_back(&gimbal);
}
