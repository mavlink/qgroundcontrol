#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(MockLinkGimbalLog)

class MockLink;

/// Simulates MAVLink Gimbal Manager Protocol for MockLink.
///
/// Supports a Gimbal Manager on MAV_COMPID_AUTOPILOT1 with a single gimbal device with the id MAV_COMP_ID_GIMBAL.
/// Gimbal attitude is simulated with a slow oscillation when not under manual control.
///
/// Gimbal capabilities are configurable via MockLink config.
///
/// Supported MAVLink messages / commands:
///   MAV_CMD_REQUEST_MESSAGE (GIMBAL_MANAGER_INFORMATION)
///   MAV_CMD_SET_MESSAGE_INTERVAL (GIMBAL_MANAGER_STATUS, GIMBAL_DEVICE_ATTITUDE_STATUS)
///   MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW
///   MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE
///   Periodic sending of GIMBAL_MANAGER_STATUS and GIMBAL_DEVICE_ATTITUDE_STATUS
class MockLinkGimbal
{
public:
    explicit MockLinkGimbal(
        MockLink *mockLink,
        bool hasRollAxis = true,
        bool hasPitchAxis = true,
        bool hasYawAxis = true,
        bool hasYawFollow = true,
        bool hasYawLock = true,
        bool hasRetract = true,
        bool hasNeutral = true);
    ~MockLinkGimbal() = default;

    /// Send periodic gimbal status messages (call from 1Hz tasks)
    void run1HzTasks();

    /// Handle all incoming MAVLink messages for gimbal.
    /// @return true if the message was handled by the gimbal
    bool handleMavlinkMessage(const mavlink_message_t &msg);

private:
    /// Handle MAV_CMD_SET_MESSAGE_INTERVAL for gimbal message IDs.
    /// @return true if the message interval was for a gimbal message
    bool _handleSetMessageInterval(const mavlink_command_long_t &request);

    /// Handle MAV_CMD_REQUEST_MESSAGE for GIMBAL_MANAGER_INFORMATION.
    /// @return true if the request was handled
    bool _handleRequestMessage(const mavlink_command_long_t &request);

    /// Handle MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW to control gimbal attitude.
    /// @return true if the command was handled
    bool _handleGimbalManagerPitchYaw(const mavlink_command_long_t &request);

    /// Handle MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE to configure gimbal manager.
    /// @return true if the command was handled
    bool _handleGimbalManagerConfigure(const mavlink_command_long_t &request);

    void _sendGimbalManagerStatus();
    void _sendGimbalDeviceAttitudeStatus();
    void _sendGimbalManagerInformation();
    void _sendCommandAck(uint16_t command, uint8_t result);

    static constexpr uint8_t kGimbalCompId = MAV_COMP_ID_GIMBAL;
    static constexpr int kDefaultIntervalUs = 1000000; // 1 Hz default

    MockLink *_mockLink = nullptr;
    int       _managerStatusIntervalUs = 0;          // 0 = disabled
    int       _deviceAttitudeStatusIntervalUs = 0;  // 0 = disabled by default
    qint64    _managerStatusLastSentMs = 0;
    qint64    _deviceAttitudeStatusLastSentMs = 0;

    // Simulated gimbal attitude (degrees)
    float     _roll = 0.0f;
    float     _pitch = 0.0f;
    float     _yaw = 0.0f;
    bool      _manualControl = false;  // true when under external control command

    // Gimbal manager configuration
    uint8_t   _gimbalManagerSysidPrimary = 0;    // System ID for primary control
    uint8_t   _gimbalManagerCompidPrimary = 0;   // Component ID for primary control
    uint8_t   _gimbalManagerSysidSecondary = 0;  // System ID for secondary control
    uint8_t   _gimbalManagerCompidSecondary = 0; // Component ID for secondary control

    // Gimbal capability flags
    bool _hasRollAxis = true;
    bool _hasPitchAxis = true;
    bool _hasYawAxis = true;
    bool _hasYawFollow = true;
    bool _hasYawLock = true;
    bool _hasRetract = true;
    bool _hasNeutral = true;
};
