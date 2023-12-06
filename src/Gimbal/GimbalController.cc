#include "GimbalController.h"

#include "MAVLinkProtocol.h"
#include "Vehicle.h"

#include <Eigen/Eigen>

QGC_LOGGING_CATEGORY(GimbalLog,         "GimbalLog")

Gimbal::Gimbal()
    : QObject(nullptr)
{
    
}

Gimbal::Gimbal(const Gimbal& other)
    : QObject(nullptr)
{
    *this = other;
}

const Gimbal& Gimbal::operator=(const Gimbal& other)
{
    requestInformationRetries = other.requestInformationRetries;                    
    requestStatusRetries      = other.requestStatusRetries;                
    requestAttitudeRetries    = other.requestAttitudeRetries;                
    deviceId                  = other.deviceId;    
    receivedInformation       = other.receivedInformation;                
    receivedStatus            = other.receivedStatus;        
    receivedAttitude          = other.receivedAttitude;            
    isComplete                = other.isComplete;    
    retracted                 = other.retracted;    
    neutral                   = other.neutral;    
    yawLock                   = other.yawLock;    
    haveControl               = other.haveControl;        
    othersHaveControl         = other.othersHaveControl;            
    _curRoll                  = other._curRoll;    
    _curPitch                 = other._curPitch;    
    _curYaw                   = other._curYaw;    

    return *this;
}

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
    if (!_potentialGimbalManagers.contains(message.compid)) {
        qCDebug(GimbalLog) << "new potential gimbal manager component: " << message.compid;
    }

    auto& gimbalManager = _potentialGimbalManagers[message.compid];

    if (!gimbalManager.receivedInformation && gimbalManager.requestGimbalManagerInformationRetries > 0) {
        _requestGimbalInformation(message.compid);
        --gimbalManager.requestGimbalManagerInformationRetries;
    }
}

void
GimbalController::_handleGimbalManagerInformation(const mavlink_message_t& message)
{

    mavlink_gimbal_manager_information_t information;
    mavlink_msg_gimbal_manager_information_decode(&message, &information);

    qCDebug(GimbalLog) << "_handleGimbalManagerInformation for gimbal device: " << information.gimbal_device_id << ", component id: " << message.compid;

    auto& gimbal = _potentialGimbals[information.gimbal_device_id];

    if (information.gimbal_device_id != 0) {
        gimbal.deviceId = information.gimbal_device_id;
    }

    if (!gimbal.receivedInformation) {
        qCDebug(GimbalLog) << "gimbal manager with compId: " << message.compid
            << " is responsible for gimbal device: " << information.gimbal_device_id;
    }

    gimbal.receivedInformation = true;

    _checkComplete(gimbal, message.compid);
}

void
GimbalController::_handleGimbalManagerStatus(const mavlink_message_t& message)
{

    mavlink_gimbal_manager_status_t status;
    mavlink_msg_gimbal_manager_status_decode(&message, &status);

    // qCDebug(GimbalLog) << "_handleGimbalManagerStatus for gimbal device: " << status.gimbal_device_id << ", component id: " << message.compid;

    auto& gimbal = _potentialGimbals[status.gimbal_device_id];

    if (!status.gimbal_device_id) {
        qCDebug(GimbalLog) << "gimbal manager with compId: " << message.compid
        << " reported status of gimbal device id: " << status.gimbal_device_id << " which is not a valid gimbal device id";
        return;
    }

    gimbal.deviceId = status.gimbal_device_id;

    // Only log this message once
    if (!gimbal.receivedStatus) {
        qCDebug(GimbalLog) << "_handleGimbalManagerStatus: gimbal manager with compId " << message.compid
            << " is responsible for gimbal device " << status.gimbal_device_id;
    }

    gimbal.receivedStatus = true;

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

    _checkComplete(gimbal, message.compid);
}

void
GimbalController::_handleGimbalDeviceAttitudeStatus(const mavlink_message_t& message)
{
    mavlink_gimbal_device_attitude_status_t attitude_status;
    mavlink_msg_gimbal_device_attitude_status_decode(&message, &attitude_status);

    uint8_t gimbal_device_id_or_compid;

    // If gimbal_device_id is 0, we must take the compid of the message
    if (attitude_status.gimbal_device_id == 0) {
        gimbal_device_id_or_compid = message.compid;

    // If the gimbal_device_id field is set to 1-6, we must use this device id instead
    } else if (attitude_status.gimbal_device_id <= 6) {
        gimbal_device_id_or_compid = attitude_status.gimbal_device_id;
    }

    // We do a reverse lookup here
    auto gimbal_it = std::find_if(_potentialGimbals.begin(), _potentialGimbals.end(),
                 [&](auto& gimbal) { return gimbal.deviceId == gimbal_device_id_or_compid; });

    if (gimbal_it == _potentialGimbals.end()) {
        qCDebug(GimbalLog) << "_handleGimbalDeviceAttitudeStatus for unknown device id: " << gimbal_device_id_or_compid << " from component id: " << message.compid;
        return;
    }

    const bool yaw_in_vehicle_frame = [&](){
        if ((attitude_status.flags & GIMBAL_DEVICE_FLAGS_YAW_IN_VEHICLE_FRAME) > 0) {
            return true;
        } else if ((attitude_status.flags & GIMBAL_DEVICE_FLAGS_YAW_IN_EARTH_FRAME) > 0) {
            return false;
        } else {
            // For backwards compatibility: if both new flags are 0, yaw lock defines the frame.
            return (attitude_status.flags & GIMBAL_DEVICE_FLAGS_YAW_LOCK) == 0;
        }
    }();

    gimbal_it->retracted = (attitude_status.flags & GIMBAL_DEVICE_FLAGS_RETRACT) > 0;
    gimbal_it->neutral = (attitude_status.flags & GIMBAL_DEVICE_FLAGS_NEUTRAL) > 0;
    gimbal_it->yawLock = (attitude_status.flags & GIMBAL_DEVICE_FLAGS_YAW_LOCK) > 0;

    auto quat = Eigen::Quaternionf{attitude_status.q[0], attitude_status.q[1], attitude_status.q[2], attitude_status.q[3]};
    auto euler = quat.toRotationMatrix().eulerAngles(2,0,1);

    gimbal_it->_curYaw   = euler[0] * (180.0f / M_PI);
    gimbal_it->_curRoll  = euler[1] * (180.0f / M_PI);
    gimbal_it->_curPitch = euler[2] * (180.0f / M_PI);

    if (yaw_in_vehicle_frame) {
        gimbal_it->_curYaw += _vehicle->heading()->rawValue().toFloat();
        if (gimbal_it->_curYaw > 180.0f) {
            gimbal_it->_curYaw -= 360.0f;
        }
    }

    // qCDebug(GimbalLog) << "roll: " << gimbal_it->_curRoll << ", pitch: " << gimbal_it->_curPitch << ", yaw: " << gimbal_it->_curYaw;

    gimbal_it->receivedAttitude = true;
    _vehicle->gimbalPitchChanged();
    _vehicle->gimbalYawChanged();

    _checkComplete(*gimbal_it, message.compid);
}

void
GimbalController::_requestGimbalInformation(uint8_t compid)
{
    qCDebug(GimbalLog) << "_requestGimbalInformation(" << compid << ")";

    if(_vehicle) {
        _vehicle->sendMavCommand(compid,
                                 MAV_CMD_REQUEST_MESSAGE,
                                 false /* no error */,
                                 MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION);
    }
}

void
GimbalController::_checkComplete(Gimbal& gimbal, uint8_t compid)
{
    if (gimbal.isComplete) {
        // Already complete, nothing to do.
        return;
    }

    if (!gimbal.receivedInformation && gimbal.requestInformationRetries > 0) {
        _requestGimbalInformation(compid);
        --gimbal.requestInformationRetries;
    }

    if (!gimbal.receivedStatus && gimbal.requestStatusRetries > 0) {
        _vehicle->sendMavCommand(compid,
                                 MAV_CMD_SET_MESSAGE_INTERVAL,
                                 false /* no error */,
                                 MAVLINK_MSG_ID_GIMBAL_MANAGER_STATUS,
                                 (gimbal.requestStatusRetries > 1) ? 0 : 5000000); // request default rate, if we don't succeed, last attempt is fixed 0.2 Hz instead
        --gimbal.requestStatusRetries;
        qCDebug(GimbalLog) << "attempt to set GIMBAL_MANAGER_STATUS message at" << (gimbal.requestStatusRetries > 1 ? "default rate" : "0.2 Hz") << "interval for device: "
                           << gimbal.deviceId << "compID: " << compid << ", retries remaining: " << gimbal.requestStatusRetries;
    }

    if (!gimbal.receivedAttitude && gimbal.requestAttitudeRetries > 0 &&
        gimbal.receivedInformation && gimbal.deviceId != 0) {
        // We request the attitude directly from the gimbal device component.
        // We can only do that once we have received the gimbal manager information
        // telling us which gimbal device it is responsible for.
        _vehicle->sendMavCommand(gimbal.deviceId,
                                 MAV_CMD_SET_MESSAGE_INTERVAL,
                                 false /* no error */,
                                 MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS,
                                 0 /* request default rate */);

        --gimbal.requestAttitudeRetries;
    }

    if (!gimbal.receivedInformation || !gimbal.receivedStatus || !gimbal.receivedAttitude) {
        // Not complete yet.
        return;
    }

    gimbal.isComplete = true;

    _gimbals.push_back(&gimbal);
    _vehicle->gimbalDataChanged();
}
