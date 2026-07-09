#pragma once

#include "PX4/px4_custom_mode.h"
#include "LinkInterface.h"
#include "MAVLinkEnums.h"
#include "MAVLinkMessageType.h"
#include "QGCMAVLinkTypes.h"
#include "MockConfiguration.h"
#include "MockLinkPX4Calibration.h"
#include "MockLinkMissionItemHandler.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtPositioning/QGeoCoordinate>

#include <array>
#include <atomic>

class MockLinkCamera;
class MockLinkFTP;
class MockLinkGimbal;
class MockLinkWorker;
class QThread;

class MockLink : public LinkInterface
{
    Q_OBJECT
    friend class MockLinkFTP;

public:
    explicit MockLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~MockLink();

    void run1HzTasks();
    void run10HzTasks();
    void run500HzTasks();
    void sendStatusTextMessages();

    bool shouldSendStatusText() const { return _sendStatusText; }

    bool isConnected() const final { return _connected; }
    void disconnect() final;

    Q_INVOKABLE void setCommLost(bool commLost) { _commLost = commLost; }
    Q_INVOKABLE void simulateConnectionRemoved();

    int vehicleId() const { return _vehicleSystemId; }
    MAV_AUTOPILOT getFirmwareType() const { return _firmwareType; }

    double vehicleLatitude() const { return _vehicleLatitude; }
    double vehicleLongitude() const { return _vehicleLongitude; }
    double vehicleAltitudeAMSL() const { return _vehicleAltitudeAMSL; }

    bool signingEnabled() const { return _signingEnabled; }

    /// Sends the specified mavlink message to QGC
    void respondWithMavlinkMessage(const mavlink_message_t &msg);

    /// Sends a STATUSTEXT message to QGC
    ///     @param severity MAV_SEVERITY value
    void sendStatusTextMessage(uint8_t severity, const QString &text);

    /// Test API: places the simulated vehicle into the given pose during calibration
    void setCalibrationPose(MockLinkPX4Calibration::Pose pose) const { _mockLinkPX4Calibration->setPose(pose); }

    MockLinkFTP *mockLinkFTP() const;

    /// Set the armed state of the simulated vehicle
    void setArmed(bool armed) { if (armed) _mavBaseMode |= MAV_MODE_FLAG_SAFETY_ARMED; else _mavBaseMode &= ~MAV_MODE_FLAG_SAFETY_ARMED; }
    bool armed() const { return (_mavBaseMode & MAV_MODE_FLAG_SAFETY_ARMED) != 0; }

    /// Sets a failure mode for unit testing
    ///     @param failureMode Type of failure to simulate
    ///     @param failureAckResult Error to send if one the ack error modes
    void setMissionItemFailureMode(MockLinkMissionItemHandler::FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult) const { _missionItemHandler->setFailureMode(failureMode, failureAckResult); }

    /// Called to send a MISSION_ACK message while the MissionManager is in idle state
    void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType) const { _missionItemHandler->sendUnexpectedMissionAck(ackType); }

    /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
    void sendUnexpectedMissionItem() const { _missionItemHandler->sendUnexpectedMissionItem(); }

    /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
    void sendUnexpectedMissionRequest() const { _missionItemHandler->sendUnexpectedMissionRequest(); }

    void sendUnexpectedCommandAck(MAV_CMD command, MAV_RESULT ackResult);

    /// Reset the state of the MissionItemHandler to no items, no transactions in progress.
    void resetMissionItemHandler() const { _missionItemHandler->reset(); }

    /// Test-only: seeds a simple multirotor mission (takeoff, waypoint, RTL) onto the simulated vehicle.
    void loadSimpleMultirotorMission() const { _missionItemHandler->loadSimpleMultirotorMission(); }

    /// Returns the filename for the simulated log file. Only available after a download is requested.
    QString logDownloadFile() const { return _logDownloadFilename; }

    void clearReceivedMavCommandCounts() { _receivedMavCommandCountMap.clear(); _receivedMavCommandByCompCountMap.clear(); _receivedRequestMessageByCompAndMsgCountMap.clear(); }
    int receivedMavCommandCount(MAV_CMD command) const { return _receivedMavCommandCountMap.value(command, 0); }
    int receivedMavCommandCount(MAV_CMD command, int compId) const { return _receivedMavCommandByCompCountMap.value(command).value(compId, 0); }
    int receivedRequestMessageCount(int compId, int messageId) const { return _receivedRequestMessageByCompAndMsgCountMap.value(compId).value(messageId, 0); }
    void clearReceivedRequestMessageCounts() { _receivedRequestMessageCountMap.clear(); _receivedRequestMessageByCompAndMsgCountMap.clear(); }
    int receivedRequestMessageCount(uint32_t messageId) const { return _receivedRequestMessageCountMap.value(messageId, 0); }
    void clearReceivedMavlinkMessageCounts() { _receivedMavlinkMessageCountMap.clear(); _lastReceivedMavlinkMessageMap.clear(); _hashCheckRequestCount = 0; _missionItemHandler->clearRequestListCounts(); }
    int receivedMavlinkMessageCount(uint32_t messageId) const { return _receivedMavlinkMessageCountMap.value(messageId, 0); }
    /// Returns the last received message with the given id. Returns false if none received.
    bool lastReceivedMavlinkMessage(uint32_t messageId, mavlink_message_t &message) const {
        if (!_lastReceivedMavlinkMessageMap.contains(messageId)) {
            return false;
        }
        message = _lastReceivedMavlinkMessageMap.value(messageId);
        return true;
    }
    int receivedMissionRequestListCount(MAV_MISSION_TYPE type) const { return _missionItemHandler->requestListCount(type); }

    enum RequestMessageFailureMode_t {
        FailRequestMessageNone,
        FailRequestMessageCommandAcceptedMsgNotSent,
        FailRequestMessageCommandUnsupported,
        FailRequestMessageCommandNoResponse,
    };
    void setRequestMessageFailureMode(RequestMessageFailureMode_t failureMode) { _requestMessageFailureMode = failureMode; }

    /// Block or unblock REQUEST_MESSAGE responses for a specific message ID.
    /// When blocked, MockLink silently drops the request (no ACK, no message).
    void setRequestMessageNoResponse(uint32_t messageId, bool noResponse = true) {
        QMutexLocker locker(&_requestMessageNoResponseMutex);
        if (noResponse) {
            _requestMessageNoResponseIds.insert(messageId);
        } else {
            _requestMessageNoResponseIds.remove(messageId);
        }
    }

    enum ParamSetFailureMode_t {
        FailParamSetNone,               ///< Normal behavior
        FailParamSetNoAck,              ///< Do not send PARAM_VALUE ack
        FailParamSetFirstAttemptNoAck,  ///< Skip ack on first attempt, respond to retry
        FailParamSetParamError,         ///< Respond with PARAM_ERROR (VALUE_OUT_OF_RANGE) instead of PARAM_VALUE
    };
    void setParamSetFailureMode(ParamSetFailureMode_t mode) {
        _paramSetFailureMode = mode;
        _paramSetFailureFirstAttemptPending = (mode == FailParamSetFirstAttemptNoAck);
    }

    enum ParamRequestReadFailureMode_t {
        FailParamRequestReadNone,               ///< Normal behavior
        FailParamRequestReadNoResponse,         ///< Do not respond to PARAM_REQUEST_READ
        FailParamRequestReadFirstAttemptNoResponse, ///< Skip response on first attempt, respond to retry
        FailParamRequestReadParamError,         ///< Respond with PARAM_ERROR (DOES_NOT_EXIST) instead of PARAM_VALUE
    };
    void setParamRequestReadFailureMode(ParamRequestReadFailureMode_t mode) {
        _paramRequestReadFailureMode = mode;
        _paramRequestReadFailureFirstAttemptPending = (mode == FailParamRequestReadFirstAttemptNoResponse);
    }

    void setHashCheckNoResponse(bool noResponse) { _hashCheckNoResponse = noResponse; }

    /// Controls whether SYS_AUTOSTART is also reset when a MAV_CMD_PREFLIGHT_STORAGE
    /// param1=2 (reset params to defaults) command is received. Defaults to false so
    /// the simulated airframe doesn't change.
    void setResetSysAutostartOnParamReset(bool reset) { _resetSysAutostartOnParamReset = reset; }

    /// Returns the number of standalone PARAM_REQUEST_READ requests for _HASH_CHECK received
    int hashCheckRequestCount() const { return _hashCheckRequestCount; }

    /// Change a float parameter value directly on MockLink (for testing cache invalidation)
    void setMockParamValue(int componentId, const QString &paramName, float value);

    /// Change an int32 parameter value directly on MockLink. Used to simulate the
    /// firmware storing calibration results (e.g. CAL_MAG0_ID).
    void setInt32ParamValue(int componentId, const QString &paramName, int32_t value) { _mapParamName2Value[componentId][paramName] = QVariant::fromValue(value); }

    /// Returns the current MockLink-side value of a parameter. Used by unit tests
    /// to verify that PARAM_SET writes reached the simulated firmware.
    QVariant paramValue(int componentId, const QString &paramName) const { return _mapParamName2Value.value(componentId).value(paramName); }

    /// Override the OPEN_DRONE_ID_ARM_STATUS the simulated RID device sends at 1Hz.
    /// status is a MAV_ODID_ARM_STATUS value.
    void setRemoteIDArmStatus(uint8_t status, const QString& error);

    static MockLink *startPX4MockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startPX4MockLinkWithMission(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startGenericMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startNoInitialConnectMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startAPMArduCopterMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startAPMArduPlaneMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startAPMArduSubMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startAPMArduRoverMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);

    // Special commands for testing Vehicle::sendMavCommandWithHandler
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED = MAV_CMD_USER_1;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED = MAV_CMD_USER_2;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED = MAV_CMD_USER_3;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED = MAV_CMD_USER_4;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_NO_RESPONSE = MAV_CMD_USER_5;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY = static_cast<MAV_CMD>(MAV_CMD_USER_5 + 1);
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_ACCEPTED = static_cast<MAV_CMD>(MAV_CMD_USER_5 + 2);
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_FAILED = static_cast<MAV_CMD>(MAV_CMD_USER_5 + 3);
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK = static_cast<MAV_CMD>(MAV_CMD_USER_5 + 4);

signals:
    void writeBytesQueuedSignal(const QByteArray &bytes);
    void highLatencyTransmissionEnabledChanged(bool highLatencyTransmissionEnabled);

private slots:
    /// Called when QGC wants to write bytes to the MAV
    void _writeBytes(const QByteArray &bytes) final;
    void _writeBytesQueued(const QByteArray &bytes);

private:
    typedef struct {
        const char *name;
        uint8_t standard_mode;
        uint32_t custom_mode;
        bool canBeSet;
        bool advanced;
    } FlightMode_t;

    bool _connect() final;
    bool _allocateMavlinkChannel() final;
    void _freeMavlinkChannel() final;

    bool _incomingMavlinkChannelIsSet() const;
    bool _outgoingMavlinkChannelIsSet() const;

    void _loadParams();
    void _resetParamsToDefaults();

    /// Convert from a parameter variant to the float value from mavlink_param_union_t
    float _floatUnionForParam(int componentId, const QString &paramName);
    uint32_t _computeParamHash(int componentId) const;
    void _setParamFloatUnionIntoMap(int componentId, const QString &paramName, float paramFloat);

    /// Handle incoming bytes which are meant to be interpreted by the NuttX shell
    void _handleIncomingNSHBytes(const char *bytes, int cBytes);
    /// Handle incoming bytes which are meant to be handled by the mavlink protocol
    void _handleIncomingMavlinkBytes(const uint8_t *bytes, int cBytes);
    /// Updates counters for received mavlink messages and commands
    void _updateIncomingMessageCounts(const mavlink_message_t &msg);
    void _handleIncomingMavlinkMsg(const mavlink_message_t &msg);
    void _handleHeartBeat(const mavlink_message_t &msg);
    void _handleSetMode(const mavlink_message_t &msg);
    void _handleParamRequestList(const mavlink_message_t &msg);
    void _handleParamSet(const mavlink_message_t &msg);
    void _handleParamRequestRead(const mavlink_message_t &msg);
    void _handleFTP(const mavlink_message_t &msg);
    void _handleCommandLong(const mavlink_message_t &msg);
    void _handleCommandInt(const mavlink_message_t &msg);
    void _handleInProgressCommandLong(const mavlink_command_long_t &request);
    void _handleCommandLongSetMessageInterval(const mavlink_command_long_t &request, bool &acccepted);
    void _handleManualControl(const mavlink_message_t &msg);
    void _handleRCChannelsOverride(const mavlink_message_t &msg);
    void _handlePreFlightCalibration(const mavlink_command_long_t &request);
    void _handleTakeoff(const mavlink_command_long_t &request);
    void _handleLogRequestList(const mavlink_message_t &msg);
    void _handleLogRequestData(const mavlink_message_t &msg);
    void _handleParamMapRC(const mavlink_message_t &msg);
    void _handleSetupSigning(const mavlink_message_t &msg);
    void _sendParamError(int componentId, const char *paramId, int16_t paramIndex, uint8_t errorCode);
    void _handleRequestMessage(const mavlink_command_long_t &request, bool &accepted, bool &noAck);
    void _handleRequestMessageAutopilotVersion(const mavlink_command_long_t &request, bool &accepted);
    void _handleRequestMessageDebug(const mavlink_command_long_t &request, bool &accepted, bool &noAck);
    void _handleRequestMessageAvailableModes(const mavlink_command_long_t &request, bool &accepted);

    void _sendHeartBeat();
    void _sendHighLatency2();
    void _sendHomePosition();
    void _sendGpsRawInt();
    void _sendGlobalPositionInt();
    void _sendExtendedSysState();
    void _sendVibration();
    void _sendSysStatus();
    void _sendBatteryStatus();
    void _sendNamedValueFloats();
    void _sendChunkedStatusText(uint16_t chunkId, bool missingChunks);
    void _sendStatusTextMessages();
    void _respondWithAutopilotVersion();
    void _sendRCChannels();
    void _sendADSBVehicles();
    void _sendGeneralMetaData();
    void _sendRemoteIDArmStatus();
    void _sendEscInfo();
    void _sendEscStatus();
    void _sendRadioStatus();
    void _sendAvailableModesMonitor();
    void _sendAttitudeQuaternion();
    void _sendAttitudeTarget();
    void _sendLocalPositionNed();
    void _sendPositionTargetLocalNed();

    void _paramRequestListWorker();
    void _logDownloadWorker();
    void _availableModesWorker();
    void _apmCompassCalWorker();
    void _apmAccelCalWorker();
    void _sendAvailableMode(uint8_t modeIndexOneBased);
    int  _availableModesCount() const;
    void _moveADSBVehicle(int vehicleIndex);

    static MockLink *_startMockLinkWorker(const QString &configName, MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType, bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode, bool preloadMission = false);
    static MockLink *_startMockLink(MockConfiguration *mockConfig);

    /// Creates a file with random contents of the specified size.
    /// @return Fully qualified path to created file
    static QString _createRandomFile(uint32_t byteCount);

    QThread *_workerThread = nullptr;
    MockLinkWorker *_worker = nullptr;

    const MockConfiguration *_mockConfig = nullptr;
    const MAV_AUTOPILOT _firmwareType = MAV_AUTOPILOT_PX4;
    const MAV_TYPE _vehicleType = MAV_TYPE_QUADROTOR;
    const bool _sendStatusText = false;
    const bool _enableCamera = false;
    const bool _enableGimbal = false;
    const MockConfiguration::FailureMode_t _failureMode = MockConfiguration::FailNone;
    const uint8_t _vehicleSystemId = 0;
    const double _vehicleLatitude = 0.0;
    const double _vehicleLongitude = 0.0;
    // These are just set for reporting the fields in _respondWithAutopilotVersion()
    // and ensuring that the Vehicle reports the fields in Vehicle::firmwareBoardVendorId etc.
    // They do not control any mock simulation (and it is up to the Custom build to do that).
    const uint16_t _boardVendorId = 0;
    const uint16_t _boardProductId = 0;
    MockLinkMissionItemHandler *const _missionItemHandler = nullptr;
    MockLinkCamera *const _mockLinkCamera = nullptr;
    MockLinkGimbal *const _mockLinkGimbal = nullptr;
    MockLinkPX4Calibration *const _mockLinkPX4Calibration = nullptr;
    MockLinkFTP *const _mockLinkFTP = nullptr;

    uint8_t _incomingMavlinkChannel = std::numeric_limits<uint8_t>::max();
    QMutex _incomingMavlinkMutex;
    /// Outgoing (vehicle-side) channel; signing state lives here, decoupled from QGC's incoming parser channel.
    uint8_t _outgoingMavlinkChannel = std::numeric_limits<uint8_t>::max();

    mavlink_signing_t _mockSigning{};
    mavlink_signing_streams_t _mockSigningStreams{};

    bool _connected = false;
    bool _inNSH = false;

    uint8_t _mavBaseMode = MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    uint32_t _mavCustomMode = PX4CustomMode::MANUAL;
    uint8_t _mavState = MAV_STATE_STANDBY;

    QElapsedTimer _runningTime;
    static constexpr int kTestParamRequestListBatch = 25;
    static constexpr int32_t _batteryMaxTimeRemaining = 15 * 60;
    int8_t _battery1PctRemaining = 100;
    int32_t _battery1TimeRemaining = _batteryMaxTimeRemaining;
    MAV_BATTERY_CHARGE_STATE _battery1ChargeState = MAV_BATTERY_CHARGE_STATE_OK;
    int8_t _battery2PctRemaining = 100;
    int32_t _battery2TimeRemaining = _batteryMaxTimeRemaining;
    MAV_BATTERY_CHARGE_STATE _battery2ChargeState = MAV_BATTERY_CHARGE_STATE_OK;

    double _vehicleAltitudeAMSL = _defaultVehicleHomeAltitude;
    std::atomic<bool> _commLost = false;
    bool _signingEnabled = false;
    bool _highLatencyTransmissionEnabled = true;

    int _sendHomePositionDelayCount = 10;               ///< No home position for 4 seconds
    int _sendGPSPositionDelayCount = 100;               ///< No gps lock for 5 seconds

    int _currentParamRequestListComponentIndex = -1;    ///< Current component index for param request list workflow, -1 for no request in progress
    int _currentParamRequestListParamIndex = -1;        ///< Current parameter index for param request list workflow
    QList<int> _paramRequestListComponentIds;           ///< Cached component IDs for param list iteration (avoids repeated keys() calls)
    QStringList _paramRequestListParamNames;            ///< Cached param names for current component (avoids repeated keys() calls)
    /// Protects _paramRequestListComponentIds and _paramRequestListParamNames from race conditions between:
    ///   - Main thread: _handleParamRequestList() modifying cache on incoming MAV_CMD_PARAM_REQUEST_LIST
    ///   - Worker thread: _paramRequestListWorker() reading cache every 2ms (500Hz)
    QMutex _paramRequestListMutex;

    // Mavlink standard modes worker information
    int _availableModesWorkerNextModeIndex = 0;         ///< 0: not active, +index: next mode the send in sequence, -index: send a single mode (indices are 1-based)
    /// Protects _availableModesWorkerNextModeIndex from check-then-set and read-modify-write races:
    ///   - Main thread: _handleRequestMessageAvailableModes() checking/starting/stopping worker
    ///   - Worker thread: _availableModesWorker() incrementing index every 2ms (500Hz)
    QMutex _availableModesWorkerMutex;
    uint8_t _availableModesMonitorSeqNumber = 0;        ///< Sequence number for the next available mode message to send

    QString _logDownloadFilename;                       ///< Filename for log download which is in progress
    uint32_t _logDownloadCurrentOffset = 0;             ///< Current offset we are sending from
    uint32_t _logDownloadBytesRemaining = 0;            ///< Number of bytes still to send, 0 = send inactive
    /// Protects log download state from race conditions between:
    ///   - Main thread: _handleLogRequestData() writing offset/count when new request arrives
    ///   - Worker thread: _logDownloadWorker() reading/modifying offset/remaining every 2ms (500Hz)
    QMutex _logDownloadMutex;

    RequestMessageFailureMode_t _requestMessageFailureMode = FailRequestMessageNone;
    mutable QMutex _requestMessageNoResponseMutex;
    QSet<uint32_t> _requestMessageNoResponseIds;
    ParamSetFailureMode_t _paramSetFailureMode = FailParamSetNone;
    bool _paramSetFailureFirstAttemptPending = false;
    ParamRequestReadFailureMode_t _paramRequestReadFailureMode = FailParamRequestReadNone;
    bool _paramRequestReadFailureFirstAttemptPending = false;
    bool _hashCheckNoResponse = false;
    int _hashCheckRequestCount = 0;
    bool _paramRequestListHashCheckSent = false;
    bool _resetSysAutostartOnParamReset = false;

    // Periodic OPEN_DRONE_ID_ARM_STATUS content (overridable via setRemoteIDArmStatus).
    // Written by the test (main) thread, read by the 1Hz worker: guarded by _remoteIDArmStatusMutex.
    QMutex _remoteIDArmStatusMutex;
    uint8_t _remoteIDArmStatus = MAV_ODID_ARM_STATUS_GOOD_TO_ARM;
    QString _remoteIDArmStatusError = QStringLiteral("No Error");

    // APM compass calibration worker state
    // Protects _apmCompassCalProgress: main thread writes 0 to start/stop,
    // worker thread reads/increments each 100ms tick.
    QMutex _apmCompassCalMutex;
    int    _apmCompassCalProgress  = -1; ///< -1 = inactive, 0-100 = progress pct
    int    _apmCompassCalTickCount = 0;  ///< 500 Hz tick counter for ~10 Hz throttle

    // APM accel calibration worker state
    // Protects _apmAccelCalPos: main thread writes the starting pos,
    // worker thread reads and advances through the sequence.
    QMutex _apmAccelCalMutex;
    /// Each position the firmware sends to QGC in sequence
    static constexpr ACCELCAL_VEHICLE_POS kAPMAccelCalPosSequence[] = {
        ACCELCAL_VEHICLE_POS_LEVEL,
        ACCELCAL_VEHICLE_POS_LEFT,
        ACCELCAL_VEHICLE_POS_RIGHT,
        ACCELCAL_VEHICLE_POS_NOSEDOWN,
        ACCELCAL_VEHICLE_POS_NOSEUP,
        ACCELCAL_VEHICLE_POS_BACK,
    };
    int  _apmAccelCalPosIndex   = -1;   ///< -1 = inactive, 0..5 = current pos, 6 = done
    bool _apmAccelCalGotAck     = false; ///< GCS clicked Next
    int  _apmAccelCalTickCount  = 0;    ///< 500 Hz tick counter for ~10 Hz throttle

    struct RCChannelOverride {
        enum class State { Ignore, Overridden, Released } state = State::Ignore;
        uint16_t value = 0;
    };
    static constexpr int kRcChannelOverrideChannelCount = 18;
    std::array<RCChannelOverride, kRcChannelOverrideChannelCount> _rcChannelOverrides;

    QMap<MAV_CMD, int> _receivedMavCommandCountMap;
    QMap<MAV_CMD, QMap<int, int>> _receivedMavCommandByCompCountMap;
    QMap<uint32_t, int> _receivedRequestMessageCountMap;
    QMap<int, QMap<int, int>> _receivedRequestMessageByCompAndMsgCountMap;
    QMap<uint32_t, int> _receivedMavlinkMessageCountMap;
    QMap<uint32_t, mavlink_message_t> _lastReceivedMavlinkMessageMap;
    QMap<int, QMap<QString, QVariant>> _mapParamName2Value;
    QMap<int, QMap<QString, MAV_PARAM_TYPE>> _mapParamName2MavParamType;

    struct ADSBVehicle {
        QGeoCoordinate coordinate;
        double angle = 0.0;
        double altitude = 0.0;                      ///< Store unique altitude for each vehicle
    };
    QList<ADSBVehicle> _adsbVehicles;               ///< Store data for multiple ADS-B vehicles
    QList<QGeoCoordinate> _adsbVehicleCoordinates;  ///< List for multiple vehicles
    static constexpr int _numberOfVehicles = 5;     ///< Number of ADS-B vehicles
    double _adsbAngles[_numberOfVehicles]{};        ///< Array for angles of each vehicle

    static std::atomic<int> _nextVehicleSystemId;

    // Vehicle position is set close to default Gazebo vehicle location. This allows for multi-vehicle
    // testing of a gazebo vehicle and a mocklink vehicle
    static constexpr double _defaultVehicleLatitude = 47.397;
    static constexpr double _defaultVehicleLongitude = 8.5455;
    static constexpr double _defaultVehicleHomeAltitude = 488.056;

    static constexpr const char *_failParam = "COM_FLTMODE6";

    static constexpr uint8_t _vehicleComponentId = MAV_COMP_ID_AUTOPILOT1;

    static constexpr uint16_t _logDownloadLogId = 0;        ///< Id of siumulated log file
    static constexpr uint32_t _logDownloadFileSize = 1000;  ///< Size of simulated log file

    static constexpr bool _mavlinkStarted = true;

    /// ArduPilot calibration-indicator parameters whose firmware default is 0 (uncalibrated).
    /// Used by _resetParamsToDefaults() and MockLinkFTP::_generateParamPck() to keep both
    /// sites in sync.
    inline static const QSet<QString> kAPMCalOffsetParams = {
        QStringLiteral("COMPASS_OFS_X"),  QStringLiteral("COMPASS_OFS_Y"),  QStringLiteral("COMPASS_OFS_Z"),
        QStringLiteral("COMPASS_OFS2_X"), QStringLiteral("COMPASS_OFS2_Y"), QStringLiteral("COMPASS_OFS2_Z"),
        QStringLiteral("COMPASS_OFS3_X"), QStringLiteral("COMPASS_OFS3_Y"), QStringLiteral("COMPASS_OFS3_Z"),
        QStringLiteral("INS_ACCOFFS_X"),  QStringLiteral("INS_ACCOFFS_Y"),  QStringLiteral("INS_ACCOFFS_Z"),
    };

    static QList<FlightMode_t> _availableFlightModes;

    std::atomic<bool> _disconnectedEmitted{false};
};
