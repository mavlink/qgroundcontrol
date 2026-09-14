#include "MockLinkGimbal.h"
#include "MAVLinkLib.h"
#include "MockLink.h"
#include "MissionCommandTree.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtMath>

#include <cmath>

QGC_LOGGING_CATEGORY(MockLinkGimbalLog, "Comms.MockLink.MockLinkGimbal")

MockLinkGimbal::MockLinkGimbal(
    MockLink *mockLink,
    bool hasRollAxis,
    bool hasPitchAxis,
    bool hasYawAxis,
    bool hasYawFollow,
    bool hasYawLock,
    bool hasRetract,
    bool hasNeutral,
    uint8_t gimbalDeviceId)
    : _mockLink(mockLink)
    , _gimbalDeviceId(gimbalDeviceId)
    , _hasRollAxis(hasRollAxis)
    , _hasPitchAxis(hasPitchAxis)
    , _hasYawAxis(hasYawAxis)
    , _hasYawFollow(hasYawFollow)
    , _hasYawLock(hasYawLock)
    , _hasRetract(hasRetract)
    , _hasNeutral(hasNeutral)
{
}

void MockLinkGimbal::run1HzTasks()
{
    // Runs every 1s (1Hz on worker thread). Reads intervals and gimbal state main thread modifies.
    // Must serialize access to prevent:
    //   - Auto-movement overwriting manual control commands
    //   - Reading stale/inconsistent interval values for status transmission
    QMutexLocker locker(&_stateMutex);
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    // Send GIMBAL_MANAGER_STATUS if interval is set
    if (_managerStatusIntervalUs > 0) {
        const qint64 intervalMs = _managerStatusIntervalUs / 1000;
        if ((nowMs - _managerStatusLastSentMs) >= intervalMs) {
            _sendGimbalManagerStatus();
            _managerStatusLastSentMs = nowMs;
        }
    }

    // Send GIMBAL_DEVICE_ATTITUDE_STATUS if interval is set
    if (_deviceAttitudeStatusIntervalUs > 0) {
        const qint64 intervalMs = _deviceAttitudeStatusIntervalUs / 1000;
        if ((nowMs - _deviceAttitudeStatusLastSentMs) >= intervalMs) {
            // Only simulate automatic movement if not under manual control
            if (!_manualControl) {
                // Simulate simple gimbal movement (slow sine wave)
                const qint64 secondsSinceEpoch = nowMs / 1000;
                _pitch = 10.0f * qSin(secondsSinceEpoch * 0.1);  // ±10° pitch oscillation
                _yaw = 15.0f * qCos(secondsSinceEpoch * 0.15);   // ±15° yaw oscillation
            }

            _sendGimbalDeviceAttitudeStatus();
            _deviceAttitudeStatusLastSentMs = nowMs;
        }
    }
}

bool MockLinkGimbal::handleMavlinkMessage(const mavlink_message_t &msg)
{
    if (msg.msgid == MAVLINK_MSG_ID_GIMBAL_MANAGER_SET_ATTITUDE) {
        _handleGimbalManagerSetAttitude(msg);
        return true;
    }

    if (msg.msgid != MAVLINK_MSG_ID_COMMAND_LONG) {
        return false;
    }

    mavlink_command_long_t request{};
    mavlink_msg_command_long_decode(&msg, &request);

    // Handle gimbal-specific commands
    switch (request.command) {
    case MAV_CMD_SET_MESSAGE_INTERVAL:
        if (_handleSetMessageInterval(request)) {
            _sendCommandAck(request.command, MAV_RESULT_ACCEPTED, request.target_component);
            return true;
        }
        return false;

    case MAV_CMD_REQUEST_MESSAGE: {
        if (static_cast<int>(request.param1) != MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION) {
            return false;
        }
        InformationResponse response;
        {
            QMutexLocker locker(&_stateMutex);
            response = _informationResponse;
        }
        switch (response) {
        case InformationResponse::Accept:
            _sendCommandAck(request.command, MAV_RESULT_ACCEPTED, request.target_component);
            _handleRequestMessage(request);
            break;
        case InformationResponse::Nack:
            _sendCommandAck(request.command, MAV_RESULT_UNSUPPORTED, request.target_component);
            break;
        case InformationResponse::Silent:
            break;
        }
        return true;
    }

    case MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW:
        if (_handleGimbalManagerPitchYaw(request)) {
            _sendCommandAck(request.command, MAV_RESULT_ACCEPTED, request.target_component);
            return true;
        }
        _sendCommandAck(request.command, MAV_RESULT_DENIED, request.target_component);
        return true;

    case MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE:
        if (_handleGimbalManagerConfigure(request, msg.sysid, msg.compid)) {
            _sendCommandAck(request.command, MAV_RESULT_ACCEPTED, request.target_component);
            return true;
        }
        _sendCommandAck(request.command, MAV_RESULT_DENIED, request.target_component);
        return true;

    default:
        return false;
    }
}

void MockLinkGimbal::_sendCommandAck(uint16_t command, uint8_t result, uint8_t sourceCompId)
{
    QString commandName = MissionCommandTree::instance()->rawName(static_cast<MAV_CMD>(command));
    qCDebug(MockLinkGimbalLog) << "Sending command ACK -" << QString("%1(%2)").arg(commandName).arg(command) << "result:" << (result == MAV_RESULT_ACCEPTED ? "ACCEPTED" : result == MAV_RESULT_DENIED ? "DENIED" : QString::number(result));

    mavlink_message_t msg{};
    (void) mavlink_msg_command_ack_pack_chan(
        _mockLink->vehicleId(),
        sourceCompId,
        _mockLink->outgoingMavlinkChannel(),
        &msg,
        command,
        result,
        0,    // progress
        0,    // result_param2
        0,    // target_system
        0     // target_component
    );
    _mockLink->respondWithMavlinkMessage(msg);
}

bool MockLinkGimbal::_handleSetMessageInterval(const mavlink_command_long_t &request)
{
    const int msgId = static_cast<int>(request.param1);
    const int intervalUs = static_cast<int>(request.param2);
    const mavlink_message_info_t* info = mavlink_get_message_info_by_id(static_cast<uint32_t>(msgId));
    QString msgName = info ? info->name : QString::number(msgId);

    qCDebug(MockLinkGimbalLog) << "SET_MESSAGE_INTERVAL -" << QString("%1(%2)").arg(msgName).arg(msgId) << "intervalUs:" << intervalUs;

    // Handle interval values per MAVLink spec:
    // -1 = disable
    // 0 = default rate
    // >0 = interval in microseconds
    int effectiveInterval = intervalUs;
    if (intervalUs == 0) {
        effectiveInterval = kDefaultIntervalUs;
    } else if (intervalUs < -1) {
        // Invalid interval
        return false;
    }

    // Thread-safe access: Main thread writing interval while worker thread reads every 1s.
    // Serialize to avoid worker using stale interval for message transmission logic.
    QMutexLocker locker(&_stateMutex);
    if (msgId == MAVLINK_MSG_ID_GIMBAL_MANAGER_STATUS) {
        _statusIntervalRequests.append(intervalUs);
        if (_ignoreStatusIntervalRequests > 0) {
            --_ignoreStatusIntervalRequests;
            qCDebug(MockLinkGimbalLog) << msgName << "interval request ignored, remaining to ignore:" << _ignoreStatusIntervalRequests;
            return true;
        }
        _managerStatusIntervalUs = effectiveInterval;
        qCDebug(MockLinkGimbalLog) << msgName << "interval set to" << effectiveInterval << "us";
        return true;
    } else if (msgId == MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS) {
        _deviceAttitudeStatusIntervalUs = effectiveInterval;
        qCDebug(MockLinkGimbalLog) << msgName << "interval set to" << effectiveInterval << "us";
        return true;
    }
    return false;
}

bool MockLinkGimbal::_handleRequestMessage(const mavlink_command_long_t &request)
{
    const int msgId = static_cast<int>(request.param1);
    const mavlink_message_info_t* info = mavlink_get_message_info_by_id(static_cast<uint32_t>(msgId));
    QString msgName = info ? info->name : QString::number(msgId);

    if (msgId == MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION) {
        qCDebug(MockLinkGimbalLog) << "REQUEST_MESSAGE -" << QString("%1(%2)").arg(msgName).arg(msgId);
        _sendGimbalManagerInformation();
        return true;
    }

    return false;
}

bool MockLinkGimbal::_handleGimbalManagerPitchYaw(const mavlink_command_long_t &request)
{
    // param1: pitch angle (degrees, positive: down, negative: up)
    // param2: yaw angle (degrees, positive: to the right, negative: to the left)
    // param3: pitch rate (deg/s)
    // param4: yaw rate (deg/s)
    // param5: flags (GIMBAL_MANAGER_FLAGS)
    // param7: gimbal device id

    const float requestedPitch = request.param1;
    const float requestedYaw = request.param2;
    const uint32_t flags = static_cast<uint32_t>(request.param5);

    qCDebug(MockLinkGimbalLog) << "DO_GIMBAL_MANAGER_PITCHYAW - pitch:" << requestedPitch << "yaw:" << requestedYaw << "flags:" << flags;

    QMutexLocker locker(&_stateMutex);
    _lastPitchYawCommand.pitchDeg = requestedPitch;
    _lastPitchYawCommand.yawDeg = requestedYaw;
    _lastPitchYawCommand.pitchRateDegS = request.param3;
    _lastPitchYawCommand.yawRateDegS = request.param4;
    _lastPitchYawCommand.flags = flags;
    _lastPitchYawCommand.deviceId = static_cast<uint8_t>(request.param7);
    ++_lastPitchYawCommand.count;

    // Flag-only commands (yaw lock / retract toggles) carry the current angles, so flags always apply
    _yawLock = (flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK) != 0;

    // Check if this is a valid request (NaN means no change requested)
    const bool updatePitch = !qIsNaN(requestedPitch);
    const bool updateYaw = !qIsNaN(requestedYaw);
    const bool updateRates = !qIsNaN(request.param3) || !qIsNaN(request.param4);

    if (!updatePitch && !updateYaw && !updateRates) {
        qCDebug(MockLinkGimbalLog) << "DO_GIMBAL_MANAGER_PITCHYAW - no valid pitch/yaw requested (both NaN)";
        return false;  // Nothing to do
    }

    // Apply limits based on gimbal capabilities
    if (updatePitch && _hasPitchAxis) {
        _pitch = qBound(-45.0f, requestedPitch, 45.0f);
        _manualControl = true;  // Switch to manual control mode
    }

    if (updateYaw && _hasYawAxis) {
        // Same frame rules as GIMBAL_DEVICE_ATTITUDE_STATUS: explicit frame flag, else YAW_LOCK means earth frame
        bool earthFrame = false;
        if (flags & GIMBAL_MANAGER_FLAGS_YAW_IN_EARTH_FRAME) {
            earthFrame = true;
        } else if (!(flags & GIMBAL_MANAGER_FLAGS_YAW_IN_VEHICLE_FRAME)) {
            earthFrame = _yawLock;
        }
        _yawInEarthFrame = earthFrame;

        // _yaw is always body yaw; an earth-frame command is convertible whenever the mock has its own heading
        float bodyYaw = qBound(-180.0f, requestedYaw, 180.0f);
        if (earthFrame && _deltaYawMode != DeltaYawMode::Legacy) {
            bodyYaw = std::remainder(requestedYaw - _deltaYawDeg, 360.0f);
        } else if (earthFrame) {
            qCDebug(MockLinkGimbalLog) << "earth-frame yaw command stored as body yaw: mock has no heading in Legacy mode";
        }
        _yaw = bodyYaw;
        _manualControl = true;  // Switch to manual control mode
    }

    qCDebug(MockLinkGimbalLog) << "Gimbal commanded to pitch:" << _pitch << "yaw:" << _yaw << "- switched to manual control";
    return true;
}

bool MockLinkGimbal::_handleGimbalManagerConfigure(const mavlink_command_long_t &request, uint8_t senderSysid, uint8_t senderCompid)
{
    // param1/2: sysid/compid primary control, param3/4: sysid/compid secondary control
    // Each field: 0 no one, -1 unchanged, -2 sender, -3 release if sender owns the pair, >0 explicit id
    // param7: gimbal device id

    QMutexLocker locker(&_stateMutex);
    _applyControlPair(request.param1, request.param2, senderSysid, senderCompid,
                      _gimbalManagerSysidPrimary, _gimbalManagerCompidPrimary);
    _applyControlPair(request.param3, request.param4, senderSysid, senderCompid,
                      _gimbalManagerSysidSecondary, _gimbalManagerCompidSecondary);

    qCDebug(MockLinkGimbalLog) << "Gimbal manager configured - Primary:" << _gimbalManagerSysidPrimary
                                << "/" << _gimbalManagerCompidPrimary
                                << "Secondary:" << _gimbalManagerSysidSecondary
                                << "/" << _gimbalManagerCompidSecondary;
    return true;
}

void MockLinkGimbal::_applyControlPair(float sysidValue, float compidValue, uint8_t senderSysid, uint8_t senderCompid,
                                       uint8_t &targetSysid, uint8_t &targetCompid)
{
    // Release acts on the pair as a whole so a non-owner can never revoke or half-clear the real controller
    if (sysidValue == -3.0f || compidValue == -3.0f) {
        if (targetSysid == senderSysid && targetCompid == senderCompid) {
            targetSysid = 0;
            targetCompid = 0;
        }
        return;
    }
    _applyControlField(sysidValue, senderSysid, targetSysid);
    _applyControlField(compidValue, senderCompid, targetCompid);
}

void MockLinkGimbal::_applyControlField(float value, uint8_t senderValue, uint8_t &target)
{
    if (qIsNaN(value) || value == -1.0f) {
        return;
    }
    if (value == -2.0f) {
        target = senderValue;
    } else if (value >= 0.0f) {
        target = static_cast<uint8_t>(value);
    }
}

void MockLinkGimbal::_handleGimbalManagerSetAttitude(const mavlink_message_t &msg)
{
    mavlink_gimbal_manager_set_attitude_t setAttitude{};
    mavlink_msg_gimbal_manager_set_attitude_decode(&msg, &setAttitude);

    qCDebug(MockLinkGimbalLog) << "GIMBAL_MANAGER_SET_ATTITUDE - pitchRate:" << setAttitude.angular_velocity_y
                               << "yawRate:" << setAttitude.angular_velocity_z
                               << "flags:" << setAttitude.flags;

    QMutexLocker locker(&_stateMutex);
    _lastSetAttitudeCommand.pitchRateDegS = qRadiansToDegrees(setAttitude.angular_velocity_y);
    _lastSetAttitudeCommand.yawRateDegS = qRadiansToDegrees(setAttitude.angular_velocity_z);
    _lastSetAttitudeCommand.flags = setAttitude.flags;
    ++_lastSetAttitudeCommand.count;
}

void MockLinkGimbal::setAttitudeDeg(float rollDeg, float pitchDeg, float yawDeg)
{
    QMutexLocker locker(&_stateMutex);
    _roll = rollDeg;
    _pitch = pitchDeg;
    _yaw = yawDeg;
    _manualControl = true;
}

void MockLinkGimbal::setDeltaYawMode(DeltaYawMode mode)
{
    QMutexLocker locker(&_stateMutex);
    _deltaYawMode = mode;
}

void MockLinkGimbal::setDeltaYawDeg(float deltaYawDeg)
{
    QMutexLocker locker(&_stateMutex);
    _deltaYawDeg = deltaYawDeg;
}

void MockLinkGimbal::setYawInEarthFrame(bool earthFrame)
{
    QMutexLocker locker(&_stateMutex);
    _yawInEarthFrame = earthFrame;
}

void MockLinkGimbal::setPrimaryControl(uint8_t sysid, uint8_t compid)
{
    QMutexLocker locker(&_stateMutex);
    _gimbalManagerSysidPrimary = sysid;
    _gimbalManagerCompidPrimary = compid;
}

void MockLinkGimbal::sendGimbalDeviceAttitudeStatusNow()
{
    QMutexLocker locker(&_stateMutex);
    _sendGimbalDeviceAttitudeStatus();
}

void MockLinkGimbal::sendGimbalManagerStatusNow()
{
    QMutexLocker locker(&_stateMutex);
    _sendGimbalManagerStatus();
}

void MockLinkGimbal::sendGimbalManagerInformationWithDeviceId(uint8_t deviceIdField)
{
    _sendGimbalManagerInformation(deviceIdField);
}

void MockLinkGimbal::sendGimbalManagerStatusWithDeviceId(uint8_t deviceIdField)
{
    QMutexLocker locker(&_stateMutex);
    _sendGimbalManagerStatus(deviceIdField);
}

void MockLinkGimbal::sendGimbalDeviceAttitudeStatusFrom(uint8_t sourceCompid, uint8_t deviceIdField)
{
    QMutexLocker locker(&_stateMutex);
    _sendGimbalDeviceAttitudeStatus(sourceCompid, deviceIdField);
}

void MockLinkGimbal::setInformationResponse(InformationResponse response)
{
    QMutexLocker locker(&_stateMutex);
    _informationResponse = response;
}

void MockLinkGimbal::setIgnoreStatusIntervalRequests(int count)
{
    QMutexLocker locker(&_stateMutex);
    _ignoreStatusIntervalRequests = count;
}

QList<int> MockLinkGimbal::statusIntervalRequests() const
{
    QMutexLocker locker(&_stateMutex);
    return _statusIntervalRequests;
}

MockLinkGimbal::PitchYawCommand MockLinkGimbal::lastPitchYawCommand() const
{
    QMutexLocker locker(&_stateMutex);
    return _lastPitchYawCommand;
}

MockLinkGimbal::SetAttitudeCommand MockLinkGimbal::lastSetAttitudeCommand() const
{
    QMutexLocker locker(&_stateMutex);
    return _lastSetAttitudeCommand;
}

bool MockLinkGimbal::yawLock() const
{
    QMutexLocker locker(&_stateMutex);
    return _yawLock;
}

void MockLinkGimbal::_sendGimbalManagerStatus()
{
    _sendGimbalManagerStatus(_gimbalDeviceId);
}

void MockLinkGimbal::_sendGimbalManagerStatus(uint8_t deviceIdField)
{
    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_MANAGER_STATUS - deviceId:" << deviceIdField;

    mavlink_message_t msg{};
    (void) mavlink_msg_gimbal_manager_status_pack_chan(
        _mockLink->vehicleId(),
        MAV_COMP_ID_AUTOPILOT1,
        _mockLink->outgoingMavlinkChannel(),
        &msg,
        0, // time_boot_ms
        0, // flags
        deviceIdField,
        _gimbalManagerSysidPrimary,
        _gimbalManagerCompidPrimary,
        _gimbalManagerSysidSecondary,
        _gimbalManagerCompidSecondary);
    _mockLink->respondWithMavlinkMessage(msg);
}

void MockLinkGimbal::_sendGimbalDeviceAttitudeStatus()
{
    _sendGimbalDeviceAttitudeStatus(_attitudeSourceCompid(), _attitudeDeviceIdField());
}

void MockLinkGimbal::_sendGimbalDeviceAttitudeStatus(uint8_t sourceCompid, uint8_t deviceIdField)
{
    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_DEVICE_ATTITUDE_STATUS - pitch:" << _pitch << "yaw:" << _yaw
                               << "manual:" << _manualControl << "deltaYawMode:" << static_cast<int>(_deltaYawMode)
                               << "deltaYaw:" << _deltaYawDeg << "earthFrame:" << _yawInEarthFrame
                               << "from:" << sourceCompid << "deviceId:" << deviceIdField;

    uint16_t flags = GIMBAL_DEVICE_FLAGS_NEUTRAL | GIMBAL_DEVICE_FLAGS_ROLL_LOCK | GIMBAL_DEVICE_FLAGS_PITCH_LOCK;
    if (_yawLock) {
        flags |= GIMBAL_DEVICE_FLAGS_YAW_LOCK;
    }

    float deltaYawRad = 0.0f;
    switch (_deltaYawMode) {
    case DeltaYawMode::Legacy:
        break;
    case DeltaYawMode::Unknown:
        flags |= _yawInEarthFrame ? GIMBAL_DEVICE_FLAGS_YAW_IN_EARTH_FRAME : GIMBAL_DEVICE_FLAGS_YAW_IN_VEHICLE_FRAME;
        deltaYawRad = NAN;
        break;
    case DeltaYawMode::Explicit:
        flags |= _yawInEarthFrame ? GIMBAL_DEVICE_FLAGS_YAW_IN_EARTH_FRAME : GIMBAL_DEVICE_FLAGS_YAW_IN_VEHICLE_FRAME;
        deltaYawRad = qDegreesToRadians(_deltaYawDeg);
        break;
    }

    // _yaw is body yaw; earth-frame reports q_earth = q_delta_yaw * q_body per the message spec. _deltaYawDeg is
    // the mock's true heading even in Unknown mode where it is transmitted as NaN.
    const float reportedYawDeg = (_yawInEarthFrame && _deltaYawMode != DeltaYawMode::Legacy) ? _yaw + _deltaYawDeg : _yaw;

    // Convert Euler angles (degrees) to quaternion
    const float rollRad = qDegreesToRadians(_roll);
    const float pitchRad = qDegreesToRadians(_pitch);
    const float yawRad = qDegreesToRadians(reportedYawDeg);

    const float cy = qCos(yawRad * 0.5f);
    const float sy = qSin(yawRad * 0.5f);
    const float cp = qCos(pitchRad * 0.5f);
    const float sp = qSin(pitchRad * 0.5f);
    const float cr = qCos(rollRad * 0.5f);
    const float sr = qSin(rollRad * 0.5f);

    float q[4];
    q[0] = cr * cp * cy + sr * sp * sy;  // w
    q[1] = sr * cp * cy - cr * sp * sy;  // x
    q[2] = cr * sp * cy + sr * cp * sy;  // y
    q[3] = cr * cp * sy - sr * sp * cy;  // z

    mavlink_message_t msg{};
    (void) mavlink_msg_gimbal_device_attitude_status_pack_chan(
        _mockLink->vehicleId(),
        sourceCompid,
        _mockLink->outgoingMavlinkChannel(),
        &msg,
        0, 0,   // target system, component
        0,      // time_boot_ms
        flags,
        q,
        0.0f, 0.0f, 0.0f,   // angular_velocity_x, y, z
        0,                  // failure flags
        deltaYawRad,
        NAN,                // delta_yaw_velocity unknown
        deviceIdField);
    _mockLink->respondWithMavlinkMessage(msg);
}

void MockLinkGimbal::_sendGimbalManagerInformation()
{
    _sendGimbalManagerInformation(_gimbalDeviceId);
}

void MockLinkGimbal::_sendGimbalManagerInformation(uint8_t deviceIdField)
{
    // Build capability flags bitmask based on configuration
    uint32_t capFlags = 0;
    if (_hasRollAxis) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_ROLL_AXIS;
    if (_hasPitchAxis) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_PITCH_AXIS;
    if (_hasYawAxis) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_AXIS;
    if (_hasYawFollow) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_FOLLOW;
    if (_hasYawLock) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_LOCK;
    if (_hasRetract) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_RETRACT;
    if (_hasNeutral) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_NEUTRAL;

    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_MANAGER_INFORMATION - capFlags:" << QString::number(capFlags, 16) << "deviceId:" << deviceIdField;

    mavlink_message_t msg{};
    (void) mavlink_msg_gimbal_manager_information_pack_chan(
        _mockLink->vehicleId(),
        MAV_COMP_ID_AUTOPILOT1,
        _mockLink->outgoingMavlinkChannel(),
        &msg,
        0, // time_boot_ms
        capFlags,
        deviceIdField,
        -45, 45,
        -45, 45,
        -180, 180);
    _mockLink->respondWithMavlinkMessage(msg);
}
