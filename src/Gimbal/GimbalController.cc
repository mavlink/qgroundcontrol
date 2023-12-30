#include "GimbalController.h"

#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

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
    receivedInformation       = other.receivedInformation;                
    receivedStatus            = other.receivedStatus;        
    receivedAttitude          = other.receivedAttitude;            
    isComplete                = other.isComplete;    
    retracted                 = other.retracted;    
    neutral                   = other.neutral;    
    yawLock                   = other.yawLock;    
    _haveControl              = other._haveControl;        
    _othersHaveControl        = other._othersHaveControl;            
    _curRoll                  = other._curRoll;    
    _curPitch                 = other._curPitch;    
    _curYaw                   = other._curYaw;  
    _haveControl              = other._haveControl;
    _othersHaveControl        = other._othersHaveControl;  
    _deviceId                 = other._deviceId;    

    return *this;
}

GimbalController::GimbalController(MAVLinkProtocol* mavlink, Vehicle* vehicle)
    : _mavlink(mavlink)
    , _vehicle(vehicle)
    , _activeGimbal(nullptr)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &GimbalController::_mavlinkMessageReceived);
}

void 
GimbalController::setActiveGimbal(Gimbal* gimbal)
{
    if (!gimbal) {
        qCDebug(GimbalLog) << "Set active gimbal: attempted to set a nullptr, returning";
        return;
    }

    if (gimbal != _activeGimbal) {
        qCDebug(GimbalLog) << "Set active gimbal: " << gimbal;
        _activeGimbal = gimbal;
        emit activeGimbalChanged();
    }
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
        gimbal.setDeviceId(information.gimbal_device_id);
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

    gimbal.setDeviceId(status.gimbal_device_id);

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

    if (gimbal.gimbalHaveControl() != haveControl) {
        gimbal.setGimbalHaveControl(haveControl);
    }

    if (gimbal.gimbalOthersHaveControl() != othersHaveControl) {
        gimbal.setGimbalOthersHaveControl(othersHaveControl);
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
                 [&](auto& gimbal) { return gimbal.deviceId() == gimbal_device_id_or_compid; });

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

    float roll, pitch, yaw;
    mavlink_quaternion_to_euler(attitude_status.q, &roll, &pitch, &yaw);

    gimbal_it->_curRoll  = roll * (180.0f / M_PI);
    gimbal_it->curRollChanged();
    gimbal_it->_curPitch = pitch * (180.0f / M_PI);
    gimbal_it->curPitchChanged();
    gimbal_it->_curYaw   = yaw * (180.0f / M_PI);
    if (yaw_in_vehicle_frame) {
        gimbal_it->_curYaw += _vehicle->heading()->rawValue().toFloat();
        if (gimbal_it->_curYaw > 180.0f) {
            gimbal_it->_curYaw -= 360.0f;
        }
    }
    gimbal_it->curYawChanged();
    // qCDebug(GimbalLog) << "roll: " << gimbal_it->_curRoll << ", pitch: " << gimbal_it->_curPitch << ", yaw: " << gimbal_it->_curYaw;

    gimbal_it->receivedAttitude = true;

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
                           << gimbal.deviceId() << "compID: " << compid << ", retries remaining: " << gimbal.requestStatusRetries;
    }

    if (!gimbal.receivedAttitude && gimbal.requestAttitudeRetries > 0 &&
        gimbal.receivedInformation && gimbal.deviceId() != 0) {
        // We request the attitude directly from the gimbal device component.
        // We can only do that once we have received the gimbal manager information
        // telling us which gimbal device it is responsible for.
        _vehicle->sendMavCommand(gimbal.deviceId(),
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

    // If there is no current active gimbal, set this one as active
    if (!_activeGimbal) {
        setActiveGimbal(&gimbal);
    }

    _gimbals.append(&gimbal);
}

bool GimbalController::_tryGetGimbalControl()
{
    if (!_activeGimbal) {
        qCDebug(GimbalLog) << "_tryGetGimbalControl: active gimbal is nullptr, returning";
        return false;
    }
    // This means other component is in control, show popup
    if (_activeGimbal->gimbalOthersHaveControl()) {
        qCDebug(GimbalLog) << "Others in control, showing popup for user to confirm control..";
        emit showAcquireGimbalControlPopup();
        return false;
    // This means nobody is in control, so we can adquire directly and attempt to control
    } else if (!_activeGimbal->gimbalHaveControl()) {
        qCDebug(GimbalLog) << "Nobody in control, adquiring ourselves..";
        acquireGimbalControl();
    }
    return true;
}

void GimbalController::gimbalControlValue(double pitch, double yaw)
{
    //qDebug() << "Gimbal:" << pitch << yaw;
    _vehicle->sendMavCommand(
                _vehicle->compId(),
                MAV_CMD_DO_MOUNT_CONTROL,
                false,                               // show errors
                static_cast<float>(pitch),           // Pitch 0 - 90
                0,                                   // Roll (not used)
                static_cast<float>(yaw),             // Yaw -180 - 180
                0,                                   // Altitude (not used)
                0,                                   // Latitude (not used)
                0,                                   // Longitude (not used)
                MAV_MOUNT_MODE_MAVLINK_TARGETING);   // MAVLink Roll,Pitch,Yaw
}

void GimbalController::gimbalPitchStep(int direction)
{
    if (!_activeGimbal) {
        qCDebug(GimbalLog) << "gimbalPitchStep: active gimbal is nullptr, returning";
        return;
    }
    //qDebug() << "Pitch:" << _curGimbalPitch << direction << (_curGimbalPitch + direction);
    double p = static_cast<double>(_activeGimbal->curPitch() + direction);
    gimbalControlValue(p, static_cast<double>(_activeGimbal->curYaw()));
}

void GimbalController::gimbalYawStep(int direction)
{
    if (!_activeGimbal) {
        qCDebug(GimbalLog) << "gimbalYawStep: active gimbal is nullptr, returning";
        return;
    }
    //qDebug() << "Yaw:" << _curGimbalYaw << direction << (_curGimbalYaw + direction);
    double y = static_cast<double>(_activeGimbal->curYaw() + direction);
    gimbalControlValue(static_cast<double>(_activeGimbal->curPitch()), y);
}
void GimbalController::centerGimbal()
{
    if (!_activeGimbal) {
        qCDebug(GimbalLog) << "gimbalYawStep: active gimbal is nullptr, returning";
        return;
    }
    gimbalControlValue(0.0, 0.0);
}

// Pan and tilt comes as +-(0-1)
void GimbalController::gimbalOnScreenControl(float panPct, float tiltPct, bool clickAndPoint, bool clickAndDrag, bool rateControl, bool retract, bool neutral, bool yawlock)
{
    if (!_activeGimbal) {
        qCDebug(GimbalLog) << "gimbalOnScreenControl: active gimbal is nullptr, returning";
        return;
    }
    if (!_tryGetGimbalControl()) {
        return;
    }
    // click and point, based on FOV
    if (clickAndPoint) {
        float hFov = qgcApp()->toolbox()->settingsManager()->gimbalControllerSettings()->CameraHFov()->rawValue().toFloat();
        float vFov = qgcApp()->toolbox()->settingsManager()->gimbalControllerSettings()->CameraVFov()->rawValue().toFloat();
        
        float panIncDesired =  panPct  * hFov;
        float tiltIncDesired = tiltPct * vFov;
        
        float panDesired = panIncDesired + _activeGimbal->curYaw() - _vehicle->heading()->rawValue().toFloat();
        qDebug() << "pan desired: " << panDesired;
        float tiltDesired = tiltIncDesired + _activeGimbal->curPitch();
        _vehicle->sendMavCommand(
            _vehicle->compId(),
            MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
            true,
            tiltDesired,
            panDesired,
            0,
            0,
            0,
            0,
            _activeGimbal->deviceId());
    
    // click and drag, based on maximum speed
    } else if (clickAndDrag) {
        // Should send rate commands, but it seems for some reason it is not working on AP side. 
        // Pitch works ok but yaw doesn't stop, it keeps like inertia, like if it was buffering the messages.
        // So we do a workaround with angle targets
        float maxSpeed = qgcApp()->toolbox()->settingsManager()->gimbalControllerSettings()->CameraSlideSpeed()->rawValue().toFloat();
        
        float panIncDesired  = panPct * maxSpeed  * 0.1f;
        float tiltIncDesired = tiltPct * maxSpeed * 0.1f;

        float panDesired = panIncDesired + _activeGimbal->curYaw();
        float tiltDesired = tiltIncDesired + _activeGimbal->curPitch();

        qDebug() << "pan: " << panPct << " " << panDesired << " tilt: "  << tiltPct << " " << tiltDesired;
        
        _vehicle->sendMavCommand(
            _vehicle->compId(),
            MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
            false,
            tiltDesired,
            panDesired,
            0,
            0,
            0,
            0,
            _activeGimbal->deviceId()); 

        //     This is how it should be
    
    //     float maxSpeed = qgcApp()->toolbox()->settingsManager()->gimbalControllerSettings()->CameraSlideSpeed()->rawValue().toFloat();
        
    //     float panDesired  = panPct * maxSpeed;
    //     float tiltDesired = tiltPct * maxSpeed;

    //     qDebug() << "pan: " << panPct << " " << panDesired << " tilt: "  << tiltPct << " " << tiltDesired;
        
    //     sendMavCommand(
    //         _vehicle->compId(),
    //         MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
    //         false,
    //         static_cast<float>(qQNaN()),
    //         static_cast<float>(qQNaN()),
    //         tiltDesired,
    //         panDesired,
    //         0,
    //         0,
    //         0);
    }
}

void GimbalController::sendGimbalManagerPitchYaw(float pan, float tilt) {
    if (!_tryGetGimbalControl()) {
        return;
    }
    unsigned flags = GIMBAL_MANAGER_FLAGS_ROLL_LOCK
        | GIMBAL_MANAGER_FLAGS_PITCH_LOCK
        | GIMBAL_MANAGER_FLAGS_YAW_IN_VEHICLE_FRAME;

    _vehicle->sendMavCommand(
                _vehicle->compId(),
                MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
                true,
                tilt,
                pan,
                0,
                0,
                flags,
                0,
                _activeGimbal->deviceId());
}

void GimbalController::setGimbalHomeTargeting()
{
    if (!_tryGetGimbalControl()) {
        return;
    }
    _vehicle->sendMavCommand(
                _vehicle->compId(),
                MAV_CMD_DO_MOUNT_CONTROL,
                false,                               // show errors
                0,                                   // Pitch 0 - 90
                0,                                   // Roll (not used)
                0,                                   // Yaw -180 - 180
                0,                                   // Altitude (not used)
                0,                                   // Latitude (not used)
                0,                                   // Longitude (not used)
                MAV_MOUNT_MODE_HOME_LOCATION);       // MAVLink Roll,Pitch,Yaw
}

void GimbalController::toggleGimbalRetracted(bool force, bool set) 
{
    if (!_tryGetGimbalControl()) {
        return;
    }
    bool setDesired;
    if (force) {
        setDesired = set;
    } else {
        setDesired = !_vehicle->gimbalRetracted();
    }


    uint32_t flags = 0;
    qDebug() << "flags before: " << flags;
    if (setDesired) {
        flags |= GIMBAL_DEVICE_FLAGS_RETRACT;
    } else 
        flags &= ~GIMBAL_DEVICE_FLAGS_RETRACT;

    qDebug() << "flags after: " << flags;
    sendGimbalManagerPitchYawFlags(flags);
}

void GimbalController::toggleGimbalNeutral(bool force, bool set) 
{
    if (!_tryGetGimbalControl()) {
        return;
    }
    bool setDesired;
    if (force) {
        setDesired = set;
    } else {
        setDesired = !_vehicle->gimbalNeutral();
    }

    uint32_t flags = 0;
    if (setDesired) {
        flags |= GIMBAL_DEVICE_FLAGS_NEUTRAL;
    } else 
        flags &= ~GIMBAL_DEVICE_FLAGS_NEUTRAL;

    sendGimbalManagerPitchYawFlags(flags);
}

// Not implemented in Ardupilot yet
void GimbalController::toggleGimbalYawLock(bool force, bool set) 
{
    if (!_tryGetGimbalControl()) {
        return;
    }
    // This needs to be generic
    if (_vehicle->apmFirmware()) {
        _vehicle->sendMavCommand(_vehicle->compId(),
            MAV_CMD_DO_AUX_FUNCTION,
            true,    // show errors
            163,     // yaw lock
            set ? 2 : 0);  // Switch position
    }

    // This is how it should be...  
    //
    // bool setDesired;
    // if (force) {
    //     setDesired = set;
    // } else {
    //     setDesired = !_gimbalYawLock;
    // }

    // uint32_t flags = 0;
    // if (setDesired) {
    //     flags |= GIMBAL_DEVICE_FLAGS_YAW_LOCK;
    // } else 
    //     flags &= ~GIMBAL_DEVICE_FLAGS_YAW_LOCK;

    // sendGimbalManagerPitchYawFlags(flags);
}

void GimbalController::setGimbalRcTargeting() 
{
    // This needs to be generic too
    if (_vehicle->apmFirmware()) {
        _vehicle->sendMavCommand(_vehicle->compId(),
            MAV_CMD_DO_AUX_FUNCTION,
            true,    // show errors
            27,      // retract mount
            0);      // low switch, sets it to default rc mode
    } else {
        // Do the standard thing for PX4.
    }
}

void GimbalController::sendGimbalManagerPitchYawFlags(uint32_t flags)
{
    // qDebug() << "flags to send: " << flags;

    _vehicle->sendMavCommand(
                _vehicle->compId(),
                MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
                true,
                static_cast<float>(qQNaN()),
                static_cast<float>(qQNaN()),
                static_cast<float>(qQNaN()),
                static_cast<float>(qQNaN()),
                flags,
                0,
                _activeGimbal->deviceId());
}

void GimbalController::acquireGimbalControl()
{
    if (!_activeGimbal) {
        qCDebug(GimbalLog) << "acquireGimbalControl: active gimbal is nullptr, returning";
        return;
    }
    _vehicle->sendMavCommand(
        _vehicle->compId(),
        MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
        true,
        _mavlink->getSystemId(), // Set us in primary control.
        _mavlink->getComponentId(), // Set us in primary control
        -1.f, // Leave secondary unchanged
        -1.f, // Leave secondary unchanged
        NAN, // Reserved
        NAN, // Reserved
        _activeGimbal->deviceId());
}

void GimbalController::releaseGimbalControl()
{
    if (!_activeGimbal) {
        qCDebug(GimbalLog) << "releaseGimbalControl: active gimbal is nullptr, returning";
        return;
    }
    _vehicle->sendMavCommand(
        _vehicle->compId(),
        MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
        true,
        -3.f, // Release primary control if we have control
        -3.f, // Release primary control if we have control
        -1.f, // Leave secondary control unchanged
        -1.f, // Leave secondary control unchanged
        NAN, // Reserved
        NAN, // Reserved
        _activeGimbal->deviceId());
}