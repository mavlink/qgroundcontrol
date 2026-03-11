#include "MockLinkGimbal.h"
#include "MockLink.h"
#include "MissionCommandTree.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtMath>

QGC_LOGGING_CATEGORY(MockLinkGimbalLog, "Comms.MockLink.MockLinkGimbal")

MockLinkGimbal::MockLinkGimbal(
    MockLink *mockLink,
    bool hasRollAxis,
    bool hasPitchAxis,
    bool hasYawAxis,
    bool hasYawFollow,
    bool hasYawLock,
    bool hasRetract,
    bool hasNeutral)
    : _mockLink(mockLink)
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
    if (msg.msgid != MAVLINK_MSG_ID_COMMAND_LONG) {
        return false;
    }

    mavlink_command_long_t request{};
    mavlink_msg_command_long_decode(&msg, &request);

    // Handle gimbal-specific commands
    switch (request.command) {
    case MAV_CMD_SET_MESSAGE_INTERVAL:
        if (_handleSetMessageInterval(request)) {
            _sendCommandAck(request.command, MAV_RESULT_ACCEPTED);
            return true;
        }
        return false;

    case MAV_CMD_REQUEST_MESSAGE:
        if (_handleRequestMessage(request)) {
            _sendCommandAck(request.command, MAV_RESULT_ACCEPTED);
            return true;
        }
        return false;

    case MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW:
        if (_handleGimbalManagerPitchYaw(request)) {
            _sendCommandAck(request.command, MAV_RESULT_ACCEPTED);
            return true;
        }
        _sendCommandAck(request.command, MAV_RESULT_DENIED);
        return true;

    case MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE:
        if (_handleGimbalManagerConfigure(request)) {
            _sendCommandAck(request.command, MAV_RESULT_ACCEPTED);
            return true;
        }
        _sendCommandAck(request.command, MAV_RESULT_DENIED);
        return true;

    default:
        return false;
    }
}

void MockLinkGimbal::_sendCommandAck(uint16_t command, uint8_t result)
{
    QString commandName = MissionCommandTree::instance()->rawName(static_cast<MAV_CMD>(command));
    qCDebug(MockLinkGimbalLog) << "Sending command ACK -" << QString("%1(%2)").arg(commandName).arg(command) << "result:" << (result == MAV_RESULT_ACCEPTED ? "ACCEPTED" : result == MAV_RESULT_DENIED ? "DENIED" : QString::number(result));

    mavlink_message_t msg{};
    (void) mavlink_msg_command_ack_pack_chan(
        _mockLink->vehicleId(),
        MAV_COMP_ID_AUTOPILOT1,
        _mockLink->mavlinkChannel(),
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

    // Check if this is a valid request (NaN means no change requested)
    const bool updatePitch = !qIsNaN(requestedPitch);
    const bool updateYaw = !qIsNaN(requestedYaw);

    if (!updatePitch && !updateYaw) {
        qCDebug(MockLinkGimbalLog) << "DO_GIMBAL_MANAGER_PITCHYAW - no valid pitch/yaw requested (both NaN)";
        return false;  // Nothing to do
    }

    // Thread-safe access: Main thread setting manual control state that worker reads every 1s.
    // Without lock: Worker could read _manualControl=false and auto-update pitch/yaw after main
    // sets them to manual values, overwriting the manual control with auto-movement.
    QMutexLocker locker(&_stateMutex);
    // Apply limits based on gimbal capabilities
    if (updatePitch && _hasPitchAxis) {
        _pitch = qBound(-45.0f, requestedPitch, 45.0f);
        _manualControl = true;  // Switch to manual control mode
    }

    if (updateYaw && _hasYawAxis) {
        // Handle yaw lock/follow modes
        if (flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK) {
            // Yaw lock mode - maintain absolute yaw
            _yaw = qBound(-180.0f, requestedYaw, 180.0f);
        } else {
            // Follow mode - yaw relative to vehicle
            _yaw = qBound(-180.0f, requestedYaw, 180.0f);
        }
        _manualControl = true;  // Switch to manual control mode
    }

    qCDebug(MockLinkGimbalLog) << "Gimbal commanded to pitch:" << _pitch << "yaw:" << _yaw << "- switched to manual control";
    return true;
}

bool MockLinkGimbal::_handleGimbalManagerConfigure(const mavlink_command_long_t &request)
{
    // param1: sysid primary control (0: no change)
    // param2: compid primary control (0: no change)
    // param3: sysid secondary control (0: no change)
    // param4: compid secondary control (0: no change)
    // param7: gimbal device id

    const uint8_t sysidPrimary = static_cast<uint8_t>(request.param1);
    const uint8_t compidPrimary = static_cast<uint8_t>(request.param2);
    const uint8_t sysidSecondary = static_cast<uint8_t>(request.param3);
    const uint8_t compidSecondary = static_cast<uint8_t>(request.param4);

    if (sysidPrimary > 0) {
        _gimbalManagerSysidPrimary = sysidPrimary;
    }
    if (compidPrimary > 0) {
        _gimbalManagerCompidPrimary = compidPrimary;
    }
    if (sysidSecondary > 0) {
        _gimbalManagerSysidSecondary = sysidSecondary;
    }
    if (compidSecondary > 0) {
        _gimbalManagerCompidSecondary = compidSecondary;
    }

    qCDebug(MockLinkGimbalLog) << "Gimbal manager configured - Primary:" << _gimbalManagerSysidPrimary
                                << "/" << _gimbalManagerCompidPrimary
                                << "Secondary:" << _gimbalManagerSysidSecondary
                                << "/" << _gimbalManagerCompidSecondary;
    return true;
}

void MockLinkGimbal::_sendGimbalManagerStatus()
{
    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_MANAGER_STATUS";

    mavlink_message_t msg{};
    (void) mavlink_msg_gimbal_manager_status_pack_chan(
        _mockLink->vehicleId(),
        MAV_COMP_ID_AUTOPILOT1,
        _mockLink->mavlinkChannel(),
        &msg,
        0, // time_boot_ms
        0, // flags
        kGimbalCompId, // gimbal_device_id
        _gimbalManagerSysidPrimary,
        _gimbalManagerCompidPrimary,
        _gimbalManagerSysidSecondary,
        _gimbalManagerCompidSecondary);
    _mockLink->respondWithMavlinkMessage(msg);
}

void MockLinkGimbal::_sendGimbalDeviceAttitudeStatus()
{
    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_DEVICE_ATTITUDE_STATUS - pitch:" << _pitch << "yaw:" << _yaw << "manual:" << _manualControl;

    // Convert Euler angles (degrees) to quaternion
    const float rollRad = qDegreesToRadians(_roll);
    const float pitchRad = qDegreesToRadians(_pitch);
    const float yawRad = qDegreesToRadians(_yaw);

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
        kGimbalCompId,
        _mockLink->mavlinkChannel(),
        &msg,
        0, 0,   // target system, component
        0,      // time_boot_ms
        GIMBAL_DEVICE_FLAGS_NEUTRAL | GIMBAL_DEVICE_FLAGS_ROLL_LOCK | GIMBAL_DEVICE_FLAGS_PITCH_LOCK,
        q,
        0.0f, 0.0f, 0.0f,   // angular_velocity_x, y, z
        0,                  // failure flags
        _pitch, _yaw,       // delta_pitch, delta_yaw (Euler angles in degrees)
        0);                 // gimbal_device_id = 0 means the device id is the same as the message component ID
    _mockLink->respondWithMavlinkMessage(msg);
}

void MockLinkGimbal::_sendGimbalManagerInformation()
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

    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_MANAGER_INFORMATION - capFlags:" << QString::number(capFlags, 16);

    mavlink_message_t msg{};
    (void) mavlink_msg_gimbal_manager_information_pack_chan(
        _mockLink->vehicleId(),
        MAV_COMP_ID_AUTOPILOT1,
        _mockLink->mavlinkChannel(),
        &msg,
        0, // time_boot_ms
        capFlags,
        kGimbalCompId,
        -45, 45,
        -45, 45,
        -180, 180);
    _mockLink->respondWithMavlinkMessage(msg);
}
