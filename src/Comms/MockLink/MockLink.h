/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "PX4/px4_custom_mode.h"
#include "LinkInterface.h"
#include "MAVLinkLib.h"
#include "MockConfiguration.h"
#include "MockLinkMissionItemHandler.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtPositioning/QGeoCoordinate>

class MockLinkFTP;
class MockLinkWorker;
class QThread;

Q_DECLARE_LOGGING_CATEGORY(MockLinkLog)
Q_DECLARE_LOGGING_CATEGORY(MockLinkVerboseLog)

class MockLink : public LinkInterface
{
    Q_OBJECT

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

    void emitRemoteControlChannelRawChanged(int channel, uint16_t raw);

    /// Sends the specified mavlink message to QGC
    void respondWithMavlinkMessage(const mavlink_message_t &msg);

    MockLinkFTP *mockLinkFTP() const;

    /// Sets a failure mode for unit testingqgcm
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

    /// Returns the filename for the simulated log file. Only available after a download is requested.
    QString logDownloadFile() const { return _logDownloadFilename; }

    void clearReceivedMavCommandCounts() { _receivedMavCommandCountMap.clear(); }
    int receivedMavCommandCount(MAV_CMD command) const { return _receivedMavCommandCountMap[command]; }

    enum RequestMessageFailureMode_t {
        FailRequestMessageNone,
        FailRequestMessageCommandAcceptedMsgNotSent,
        FailRequestMessageCommandUnsupported,
        FailRequestMessageCommandNoResponse,
    };
    void setRequestMessageFailureMode(RequestMessageFailureMode_t failureMode) { _requestMessageFailureMode = failureMode; }

    static MockLink *startPX4MockLink(bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startGenericMockLink(bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startNoInitialConnectMockLink(bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startAPMArduCopterMockLink(bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startAPMArduPlaneMockLink(bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startAPMArduSubMockLink(bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink *startAPMArduRoverMockLink(bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);

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
    bool _connect() final;
    bool _allocateMavlinkChannel() final;
    void _freeMavlinkChannel() final;

    uint8_t _getMavlinkAuxChannel() const { return _mavlinkAuxChannel; }
    bool _mavlinkAuxChannelIsSet() const;

    void _loadParams();

    /// Convert from a parameter variant to the float value from mavlink_param_union_t
    float _floatUnionForParam(int componentId, const QString &paramName);
    void _setParamFloatUnionIntoMap(int componentId, const QString &paramName, float paramFloat);

    /// Handle incoming bytes which are meant to be interpreted by the NuttX shell
    void _handleIncomingNSHBytes(const char *bytes, int cBytes);
    /// Handle incoming bytes which are meant to be handled by the mavlink protocol
    void _handleIncomingMavlinkBytes(const uint8_t *bytes, int cBytes);
    void _handleIncomingMavlinkMsg(const mavlink_message_t &msg);
    void _handleHeartBeat(const mavlink_message_t &msg);
    void _handleSetMode(const mavlink_message_t &msg);
    void _handleParamRequestList(const mavlink_message_t &msg);
    void _handleParamSet(const mavlink_message_t &msg);
    void _handleParamRequestRead(const mavlink_message_t &msg);
    void _handleFTP(const mavlink_message_t &msg);
    void _handleCommandLong(const mavlink_message_t &msg);
    void _handleInProgressCommandLong(const mavlink_command_long_t &request);
    void _handleManualControl(const mavlink_message_t &msg);
    void _handlePreFlightCalibration(const mavlink_command_long_t &request);
    void _handleTakeoff(const mavlink_command_long_t &request);
    void _handleLogRequestList(const mavlink_message_t &msg);
    void _handleLogRequestData(const mavlink_message_t &msg);
    void _handleParamMapRC(const mavlink_message_t &msg);
    bool _handleRequestMessage(const mavlink_command_long_t &request, bool &noAck);

    void _sendHeartBeat();
    void _sendHighLatency2();
    void _sendHomePosition();
    void _sendGpsRawInt();
    void _sendGlobalPositionInt();
    void _sendExtendedSysState();
    void _sendVibration();
    void _sendSysStatus();
    void _sendBatteryStatus();
    void _sendChunkedStatusText(uint16_t chunkId, bool missingChunks);
    void _sendStatusTextMessages();
    void _respondWithAutopilotVersion();
    void _sendRCChannels();
    /// Sends the next parameter to the vehicle
    void _sendADSBVehicles();
    void _sendGeneralMetaData();
    void _sendRemoteIDArmStatus();

    void _paramRequestListWorker();
    void _logDownloadWorker();
    void _moveADSBVehicle(int vehicleIndex);

    static MockLink *_startMockLinkWorker(const QString &configName, MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType, bool sendStatusText, MockConfiguration::FailureMode_t failureMode);
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
    MockLinkFTP *const _mockLinkFTP = nullptr;

    uint8_t _mavlinkAuxChannel = std::numeric_limits<uint8_t>::max();
    QMutex _mavlinkAuxMutex;

    bool _connected = false;
    bool _inNSH = false;

    uint8_t _mavBaseMode = MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    uint32_t _mavCustomMode = PX4CustomMode::MANUAL;
    uint8_t _mavState = MAV_STATE_STANDBY;

    QElapsedTimer _runningTime;
    static constexpr int32_t _batteryMaxTimeRemaining = 15 * 60;
    int8_t _battery1PctRemaining = 100;
    int32_t _battery1TimeRemaining = _batteryMaxTimeRemaining;
    MAV_BATTERY_CHARGE_STATE _battery1ChargeState = MAV_BATTERY_CHARGE_STATE_OK;
    int8_t _battery2PctRemaining = 100;
    int32_t _battery2TimeRemaining = _batteryMaxTimeRemaining;
    MAV_BATTERY_CHARGE_STATE _battery2ChargeState = MAV_BATTERY_CHARGE_STATE_OK;

    double _vehicleAltitudeAMSL = _defaultVehicleHomeAltitude;
    bool _commLost = false;
    bool _highLatencyTransmissionEnabled = true;

    int _sendHomePositionDelayCount = 10;               ///< No home position for 4 seconds
    int _sendGPSPositionDelayCount = 100;               ///< No gps lock for 5 seconds

    int _currentParamRequestListComponentIndex = -1;    ///< Current component index for param request list workflow, -1 for no request in progress
    int _currentParamRequestListParamIndex = -1;        ///< Current parameter index for param request list workflow

    QString _logDownloadFilename;                       ///< Filename for log download which is in progress
    uint32_t _logDownloadCurrentOffset = 0;             ///< Current offset we are sending from
    uint32_t _logDownloadBytesRemaining = 0;            ///< Number of bytes still to send, 0 = send inactive

    RequestMessageFailureMode_t _requestMessageFailureMode = FailRequestMessageNone;

    QMap<MAV_CMD, int> _receivedMavCommandCountMap;
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

    static int _nextVehicleSystemId;

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
};
