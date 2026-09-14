#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QList>
#include <QtCore/QMutex>

#include <cmath>

class MockLink;

/// \brief Simulates MAVLink Gimbal Manager Protocol for MockLink.
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
///   GIMBAL_MANAGER_SET_ATTITUDE (rate control)
///   Periodic sending of GIMBAL_MANAGER_STATUS and GIMBAL_DEVICE_ATTITUDE_STATUS
///
class MockLinkGimbal
{
public:
    /// Controls how GIMBAL_DEVICE_ATTITUDE_STATUS.delta_yaw and the yaw frame flags are reported.
    enum class DeltaYawMode {
        Legacy,     ///< No YAW_IN_*_FRAME flag, delta_yaw = 0 (pre-extension firmware wire image)
        Unknown,    ///< Frame flag set, delta_yaw = NaN
        Explicit,   ///< Frame flag set, delta_yaw = value from setDeltaYawDeg()
    };

    /// Last MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW received (angles in degrees, rates in deg/s)
    struct PitchYawCommand {
        float pitchDeg = NAN;
        float yawDeg = NAN;
        float pitchRateDegS = NAN;
        float yawRateDegS = NAN;
        uint32_t flags = 0;
        uint8_t deviceId = 0;
        int count = 0;
    };

    /// Last GIMBAL_MANAGER_SET_ATTITUDE received (rates in deg/s)
    struct SetAttitudeCommand {
        float pitchRateDegS = NAN;
        float yawRateDegS = NAN;
        uint32_t flags = 0;
        int count = 0;
    };

    /// How the manager answers MAV_CMD_REQUEST_MESSAGE(GIMBAL_MANAGER_INFORMATION)
    enum class InformationResponse {
        Accept,     ///< ACK and send the message
        Nack,       ///< ACK with MAV_RESULT_UNSUPPORTED, no message
        Silent,     ///< Swallow the request, no ACK, no message
    };

    explicit MockLinkGimbal(
        MockLink *mockLink,
        bool hasRollAxis = true,
        bool hasPitchAxis = true,
        bool hasYawAxis = true,
        bool hasYawFollow = true,
        bool hasYawLock = true,
        bool hasRetract = true,
        bool hasNeutral = true,
        uint8_t gimbalDeviceId = MAV_COMP_ID_GIMBAL);
    ~MockLinkGimbal() = default;

    /// Send periodic gimbal status messages (call from 1Hz tasks)
    void run1HzTasks();

    /// Handle all incoming MAVLink messages for gimbal.
    /// @return true if the message was handled by the gimbal
    bool handleMavlinkMessage(const mavlink_message_t &msg);

    // Test APIs. All are safe to call from the test thread while the worker runs 1Hz tasks.

    /// Pins gimbal attitude (degrees) and disables the automatic oscillation.
    void setAttitudeDeg(float rollDeg, float pitchDeg, float yawDeg);
    void setDeltaYawMode(DeltaYawMode mode);
    void setDeltaYawDeg(float deltaYawDeg);
    /// Reporting frame for yaw: flags carry YAW_IN_EARTH_FRAME and q encodes yaw + delta_yaw. A PITCHYAW command
    /// overrides this with its own frame flags.
    void setYawInEarthFrame(bool earthFrame);
    /// Overrides who holds primary control as reported in GIMBAL_MANAGER_STATUS.
    void setPrimaryControl(uint8_t sysid, uint8_t compid);
    /// Sends GIMBAL_DEVICE_ATTITUDE_STATUS immediately instead of waiting for the 1Hz worker.
    void sendGimbalDeviceAttitudeStatusNow();
    void sendGimbalManagerStatusNow();

    /// Raw sends with protocol-violating field overrides, for exercising QGC's rejection paths.
    void sendGimbalManagerInformationWithDeviceId(uint8_t deviceIdField);
    void sendGimbalManagerStatusWithDeviceId(uint8_t deviceIdField);
    void sendGimbalDeviceAttitudeStatusFrom(uint8_t sourceCompid, uint8_t deviceIdField);

    void setInformationResponse(InformationResponse response);
    /// The first @a count MAV_CMD_SET_MESSAGE_INTERVAL requests for GIMBAL_MANAGER_STATUS are ACKed but not honoured.
    void setIgnoreStatusIntervalRequests(int count);
    /// Every GIMBAL_MANAGER_STATUS interval value (us) received, in order.
    QList<int> statusIntervalRequests() const;

    PitchYawCommand lastPitchYawCommand() const;
    SetAttitudeCommand lastSetAttitudeCommand() const;
    bool yawLock() const;
    uint8_t gimbalDeviceId() const { return _gimbalDeviceId; }

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
    bool _handleGimbalManagerConfigure(const mavlink_command_long_t &request, uint8_t senderSysid, uint8_t senderCompid);

    void _handleGimbalManagerSetAttitude(const mavlink_message_t &msg);

    void _sendGimbalManagerStatus();
    void _sendGimbalManagerStatus(uint8_t deviceIdField);
    void _sendGimbalDeviceAttitudeStatus();
    void _sendGimbalDeviceAttitudeStatus(uint8_t sourceCompid, uint8_t deviceIdField);
    void _sendGimbalManagerInformation();
    void _sendGimbalManagerInformation(uint8_t deviceIdField);
    void _sendCommandAck(uint16_t command, uint8_t result, uint8_t sourceCompId);

    /// Applies one MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE sysid/compid pair; -3 releases only when the sender owns the pair
    static void _applyControlPair(float sysidValue, float compidValue, uint8_t senderSysid, uint8_t senderCompid,
                                  uint8_t &targetSysid, uint8_t &targetCompid);
    /// Single control field: 0 no one, -1/NaN unchanged, -2 sender, >0 explicit id (-3 is handled by _applyControlPair)
    static void _applyControlField(float value, uint8_t senderValue, uint8_t &target);

    /// Attitude status source compid and gimbal_device_id field per the gimbal v2 addressing rules
    uint8_t _attitudeSourceCompid() const { return (_gimbalDeviceId <= 6) ? static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1) : _gimbalDeviceId; }
    uint8_t _attitudeDeviceIdField() const { return (_gimbalDeviceId <= 6) ? _gimbalDeviceId : 0; }

    static constexpr int kDefaultIntervalUs = 1000000; // 1 Hz default

    MockLink *_mockLink = nullptr;
    const uint8_t _gimbalDeviceId = MAV_COMP_ID_GIMBAL;
    InformationResponse _informationResponse = InformationResponse::Accept;
    int       _ignoreStatusIntervalRequests = 0;
    QList<int> _statusIntervalRequests;
    int       _managerStatusIntervalUs = 0;          // 0 = disabled
    int       _deviceAttitudeStatusIntervalUs = 0;  // 0 = disabled by default
    qint64    _managerStatusLastSentMs = 0;
    qint64    _deviceAttitudeStatusLastSentMs = 0;

    // Simulated gimbal attitude (degrees)
    float     _roll = 0.0f;
    float     _pitch = 0.0f;
    float     _yaw = 0.0f;
    bool      _manualControl = false;  // true when under external control command
    bool      _yawLock = false;
    bool      _yawInEarthFrame = false;
    DeltaYawMode _deltaYawMode = DeltaYawMode::Unknown;
    float     _deltaYawDeg = 0.0f;
    PitchYawCommand _lastPitchYawCommand;
    SetAttitudeCommand _lastSetAttitudeCommand;
    /// Protects gimbal state and intervals from race conditions between:
    ///   - Main thread: _handleSetMessageInterval() and _handleGimbalManagerPitchYaw() modifying state
    ///   - Worker thread: run1HzTasks() reading intervals and auto-updating attitude every 1s
    ///   Race example: Main sets _manualControl=true, Worker reads false and overwrites manual commands
    mutable QMutex _stateMutex;

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
