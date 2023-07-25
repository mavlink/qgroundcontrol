/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QLoggingCategory>
#include <QMap>
#include <QMutex>

#include "MockLinkMissionItemHandler.h"
#include "MockLinkFTP.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(MockLinkLog)
Q_DECLARE_LOGGING_CATEGORY(MockLinkVerboseLog)

class LinkManager;

class MockConfiguration : public LinkConfiguration
{
    Q_OBJECT

public:
    MockConfiguration(const QString& name);
    MockConfiguration(MockConfiguration* source);

    Q_PROPERTY(int      firmware            READ firmware           WRITE setFirmware           NOTIFY firmwareChanged)
    Q_PROPERTY(int      vehicle             READ vehicle            WRITE setVehicle            NOTIFY vehicleChanged)
    Q_PROPERTY(bool     sendStatus          READ sendStatusText     WRITE setSendStatusText     NOTIFY sendStatusChanged)
    Q_PROPERTY(bool     incrementVehicleId  READ incrementVehicleId WRITE setIncrementVehicleId NOTIFY incrementVehicleIdChanged)

    int     firmware                (void)                      { return (int)_firmwareType; }
    void    setFirmware             (int type)                  { _firmwareType = (MAV_AUTOPILOT)type; emit firmwareChanged(); }
    int     vehicle                 (void)                      { return (int)_vehicleType; }
    bool    incrementVehicleId      (void) const                     { return _incrementVehicleId; }
    void    setVehicle              (int type)                  { _vehicleType = (MAV_TYPE)type; emit vehicleChanged(); }
    void    setIncrementVehicleId   (bool incrementVehicleId)   { _incrementVehicleId = incrementVehicleId; emit incrementVehicleIdChanged(); }


    MAV_AUTOPILOT   firmwareType        (void)                          { return _firmwareType; }
    uint16_t        boardVendorId       (void)                          { return _boardVendorId; }
    uint16_t        boardProductId      (void)                          { return _boardProductId; }
    MAV_TYPE        vehicleType         (void)                          { return _vehicleType; }
    bool            sendStatusText      (void) const                         { return _sendStatusText; }

    void            setFirmwareType     (MAV_AUTOPILOT firmwareType)    { _firmwareType = firmwareType; emit firmwareChanged(); }
    void            setBoardVendorProduct(uint16_t vendorId, uint16_t productId) { _boardVendorId = vendorId; _boardProductId = productId; }
    void            setVehicleType      (MAV_TYPE vehicleType)          { _vehicleType = vehicleType; emit vehicleChanged(); }
    void            setSendStatusText   (bool sendStatusText)           { _sendStatusText = sendStatusText; emit sendStatusChanged(); }

    typedef enum {
        FailNone,                                                   // No failures
        FailParamNoReponseToRequestList,                            // Do no respond to PARAM_REQUEST_LIST
        FailMissingParamOnInitialReqest,                            // Not all params are sent on initial request, should still succeed since QGC will re-query missing params
        FailMissingParamOnAllRequests,                              // Not all params are sent on initial request, QGC retries will fail as well
        FailInitialConnectRequestMessageAutopilotVersionFailure,    // REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure
        FailInitialConnectRequestMessageAutopilotVersionLost,       // REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent
        FailInitialConnectRequestMessageProtocolVersionFailure,     // REQUEST_MESSAGE:PROTOCOL_VERSION returns failure
        FailInitialConnectRequestMessageProtocolVersionLost,        // REQUEST_MESSAGE:PROTOCOL_VERSION success, PROTOCOL_VERSION never sent
    } FailureMode_t;
    FailureMode_t failureMode(void) { return _failureMode; }
    void setFailureMode(FailureMode_t failureMode) { _failureMode = failureMode; }

    // Overrides from LinkConfiguration
    LinkType    type            (void) override                                         { return LinkConfiguration::TypeMock; }
    void        copyFrom        (LinkConfiguration* source) override;
    void        loadSettings    (QSettings& settings, const QString& root) override;
    void        saveSettings    (QSettings& settings, const QString& root) override;
    QString     settingsURL     (void) override                                         { return "MockLinkSettings.qml"; }
    QString     settingsTitle   (void) override                                         { return tr("Mock Link Settings"); }

signals:
    void firmwareChanged            (void);
    void vehicleChanged             (void);
    void sendStatusChanged          (void);
    void incrementVehicleIdChanged  (void);

private:
    MAV_AUTOPILOT   _firmwareType       = MAV_AUTOPILOT_PX4;
    MAV_TYPE        _vehicleType        = MAV_TYPE_QUADROTOR;
    bool            _sendStatusText     = false;
    FailureMode_t   _failureMode        = FailNone;
    bool            _incrementVehicleId = true;
    uint16_t        _boardVendorId      = 0;
    uint16_t        _boardProductId     = 0;

    static const char* _firmwareTypeKey;
    static const char* _vehicleTypeKey;
    static const char* _sendStatusTextKey;
    static const char* _incrementVehicleIdKey;
    static const char* _failureModeKey;
};

class MockLink : public LinkInterface
{
    Q_OBJECT

public:
    MockLink(SharedLinkConfigurationPtr& config);
    virtual ~MockLink();

    int             vehicleId           (void) const                                         { return _vehicleSystemId; }
    MAV_AUTOPILOT   getFirmwareType     (void)                                          { return _firmwareType; }
    void            setFirmwareType     (MAV_AUTOPILOT autopilot)                       { _firmwareType = autopilot; }
    void            setSendStatusText   (bool sendStatusText)                           { _sendStatusText = sendStatusText; }
    void            setFailureMode      (MockConfiguration::FailureMode_t failureMode)  { _failureMode = failureMode; }

    /// APM stack has strange handling of the first item of the mission list. If it has no
    /// onboard mission items, sometimes it sends back a home position in position 0 and
    /// sometimes it doesn't. Don't ask. This option allows you to configure that behavior
    /// for unit testing.
    void setAPMMissionResponseMode(bool sendHomePositionOnEmptyList) { _apmSendHomePositionOnEmptyList = sendHomePositionOnEmptyList; }

    void emitRemoteControlChannelRawChanged(int channel, uint16_t raw);

    /// Sends the specified mavlink message to QGC
    void respondWithMavlinkMessage(const mavlink_message_t& msg);

    MockLinkFTP* mockLinkFTP(void) { return _mockLinkFTP; }

    // Overrides from LinkInterface
    bool isConnected(void) const override { return _connected; }
    void disconnect (void) override;

    /// Sets a failure mode for unit testingqgcm
    ///     @param failureMode Type of failure to simulate
    ///     @param failureAckResult Error to send if one the ack error modes
    void setMissionItemFailureMode(MockLinkMissionItemHandler::FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult);

    /// Called to send a MISSION_ACK message while the MissionManager is in idle state
    void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType) { _missionItemHandler.sendUnexpectedMissionAck(ackType); }

    /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
    void sendUnexpectedMissionItem(void) { _missionItemHandler.sendUnexpectedMissionItem(); }

    /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
    void sendUnexpectedMissionRequest(void) { _missionItemHandler.sendUnexpectedMissionRequest(); }

    void sendUnexpectedCommandAck(MAV_CMD command, MAV_RESULT ackResult);

    /// Reset the state of the MissionItemHandler to no items, no transactions in progress.
    void resetMissionItemHandler(void) { _missionItemHandler.reset(); }

    /// Returns the filename for the simulated log file. Only available after a download is requested.
    QString logDownloadFile(void) { return _logDownloadFilename; }

    Q_INVOKABLE void setCommLost                    (bool commLost)   { _commLost = commLost; }
    Q_INVOKABLE void simulateConnectionRemoved      (void);
    static MockLink* startPX4MockLink               (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startGenericMockLink           (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startNoInitialConnectMockLink  (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startAPMArduCopterMockLink     (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startAPMArduPlaneMockLink      (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startAPMArduSubMockLink        (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startAPMArduRoverMockLink      (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);

    // Special commands for testing COMMAND_LONG handlers. By default all commands except for MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY should retry.
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED            = MAV_CMD_USER_1;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED              = MAV_CMD_USER_2;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED    = MAV_CMD_USER_3;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED      = MAV_CMD_USER_4;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_NO_RESPONSE                       = MAV_CMD_USER_5;
    static constexpr MAV_CMD MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY              = static_cast<MAV_CMD>(MAV_CMD_USER_5 + 1);

    void clearSendMavCommandCounts(void) { _sendMavCommandCountMap.clear(); }
    int sendMavCommandCount(MAV_CMD command) { return _sendMavCommandCountMap[command]; }

    typedef enum {
        FailRequestMessageNone,
        FailRequestMessageCommandAcceptedMsgNotSent,
        FailRequestMessageCommandUnsupported,
        FailRequestMessageCommandNoResponse,
    } RequestMessageFailureMode_t;
    void setRequestMessageFailureMode(RequestMessageFailureMode_t failureMode) { _requestMessageFailureMode = failureMode; }

signals:
    void writeBytesQueuedSignal                 (const QByteArray bytes);
    void highLatencyTransmissionEnabledChanged  (bool highLatencyTransmissionEnabled);

private slots:
    // LinkInterface overrides
    void _writeBytes(const QByteArray bytes) final;

    void _writeBytesQueued(const QByteArray bytes);
    void _run1HzTasks(void);
    void _run10HzTasks(void);
    void _run500HzTasks(void);

private:
    // LinkInterface overrides
    bool _connect                       (void) override;
    bool _allocateMavlinkChannel        () override;
    void _freeMavlinkChannel            () override;
    uint8_t mavlinkAuxChannel           (void) const;
    bool mavlinkAuxChannelIsSet         (void) const;

    // QThread override
    void run(void) final;

    // MockLink methods
    void _sendHeartBeat                 (void);
    void _sendHighLatency2              (void);
    void _handleIncomingNSHBytes        (const char* bytes, int cBytes);
    void _handleIncomingMavlinkBytes    (const uint8_t* bytes, int cBytes);
    void _handleIncomingMavlinkMsg      (const mavlink_message_t& msg);
    void _loadParams                    (void);
    void _handleHeartBeat               (const mavlink_message_t& msg);
    void _handleSetMode                 (const mavlink_message_t& msg);
    void _handleParamRequestList        (const mavlink_message_t& msg);
    void _handleParamSet                (const mavlink_message_t& msg);
    void _handleParamRequestRead        (const mavlink_message_t& msg);
    void _handleFTP                     (const mavlink_message_t& msg);
    void _handleCommandLong             (const mavlink_message_t& msg);
    void _handleManualControl           (const mavlink_message_t& msg);
    void _handlePreFlightCalibration    (const mavlink_command_long_t& request);
    void _handleLogRequestList          (const mavlink_message_t& msg);
    void _handleLogRequestData          (const mavlink_message_t& msg);
    void _handleParamMapRC              (const mavlink_message_t& msg);
    bool _handleRequestMessage          (const mavlink_command_long_t& request, bool& noAck);
    float _floatUnionForParam           (int componentId, const QString& paramName);
    void _setParamFloatUnionIntoMap     (int componentId, const QString& paramName, float paramFloat);
    void _sendHomePosition              (void);
    void _sendGpsRawInt                 (void);
    void _sendVibration                 (void);
    void _sendSysStatus                 (void);
    void _sendBatteryStatus             (void);
    void _sendStatusTextMessages        (void);
    void _sendChunkedStatusText         (uint16_t chunkId, bool missingChunks);
    void _respondWithAutopilotVersion   (void);
    void _sendRCChannels                (void);
    void _paramRequestListWorker        (void);
    void _logDownloadWorker             (void);
    void _sendADSBVehicles              (void);
    void _moveADSBVehicle               (void);
    void _sendGeneralMetaData           (void);

    static MockLink* _startMockLinkWorker(QString configName, MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType, bool sendStatusText, MockConfiguration::FailureMode_t failureMode);
    static MockLink* _startMockLink(MockConfiguration* mockConfig);

    uint8_t                     _mavlinkAuxChannel              = std::numeric_limits<uint8_t>::max();
    QMutex                      _mavlinkAuxMutex;

    MockLinkMissionItemHandler  _missionItemHandler;

    QString                     _name;
    bool                        _connected;

    uint8_t                     _vehicleSystemId;
    uint8_t                     _vehicleComponentId             = MAV_COMP_ID_AUTOPILOT1;

    bool                        _inNSH;
    bool                        _mavlinkStarted;

    uint8_t                     _mavBaseMode;
    uint32_t                    _mavCustomMode;
    uint8_t                     _mavState;

    QElapsedTimer               _runningTime;
    static const int32_t        _batteryMaxTimeRemaining        = 15 * 60;
    int8_t                      _battery1PctRemaining           = 100;
    int32_t                     _battery1TimeRemaining          = _batteryMaxTimeRemaining;
    MAV_BATTERY_CHARGE_STATE    _battery1ChargeState            = MAV_BATTERY_CHARGE_STATE_OK;
    int8_t                      _battery2PctRemaining           = 100;
    int32_t                     _battery2TimeRemaining          = _batteryMaxTimeRemaining;
    MAV_BATTERY_CHARGE_STATE    _battery2ChargeState            = MAV_BATTERY_CHARGE_STATE_OK;

    MAV_AUTOPILOT               _firmwareType;
    MAV_TYPE                    _vehicleType;
    double                      _vehicleLatitude;
    double                      _vehicleLongitude;
    double                      _vehicleAltitude;
    bool                        _commLost                       = false;
    bool                        _highLatencyTransmissionEnabled = true;

    // These are just set for reporting the fields in _respondWithAutopilotVersion()
    // and ensuring that the Vehicle reports the fields in Vehicle::firmwareBoardVendorId etc.
    // They do not control any mock simulation (and it is up to the Custom build to do that).
    uint16_t                    _boardVendorId      = 0;
    uint16_t                    _boardProductId     = 0;

    MockLinkFTP* _mockLinkFTP = nullptr;

    bool _sendStatusText;
    bool _apmSendHomePositionOnEmptyList;
    MockConfiguration::FailureMode_t _failureMode;

    int _sendHomePositionDelayCount;
    int _sendGPSPositionDelayCount;

    int _currentParamRequestListComponentIndex; // Current component index for param request list workflow, -1 for no request in progress
    int _currentParamRequestListParamIndex;     // Current parameter index for param request list workflow

    static const uint16_t _logDownloadLogId = 0;        ///< Id of siumulated log file
    static const uint32_t _logDownloadFileSize = 1000;  ///< Size of simulated log file

    QString     _logDownloadFilename;       ///< Filename for log download which is in progress
    uint32_t    _logDownloadCurrentOffset;  ///< Current offset we are sending from
    uint32_t    _logDownloadBytesRemaining; ///< Number of bytes still to send, 0 = send inactive

    QGeoCoordinate  _adsbVehicleCoordinate;
    double          _adsbAngle;

    RequestMessageFailureMode_t _requestMessageFailureMode = FailRequestMessageNone;

    QMap<MAV_CMD, int>  _sendMavCommandCountMap;
    QMap<int, QMap<QString, QVariant>>          _mapParamName2Value;
    QMap<int, QMap<QString, MAV_PARAM_TYPE>>    _mapParamName2MavParamType;

    static double       _defaultVehicleLatitude;
    static double       _defaultVehicleLongitude;
    static double       _defaultVehicleAltitude;
    static int          _nextVehicleSystemId;
    static const char*  _failParam;
};

