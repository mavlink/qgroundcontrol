/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MOCKLINK_H
#define MOCKLINK_H

#include <QMap>
#include <QLoggingCategory>

#include "MockLinkMissionItemHandler.h"
#include "MockLinkFileServer.h"
#include "LinkManager.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(MockLinkLog)
Q_DECLARE_LOGGING_CATEGORY(MockLinkVerboseLog)

class MockConfiguration : public LinkConfiguration
{
    Q_OBJECT

public:

    Q_PROPERTY(int      firmware    READ firmware           WRITE setFirmware       NOTIFY firmwareChanged)
    Q_PROPERTY(int      vehicle     READ vehicle            WRITE setVehicle        NOTIFY vehicleChanged)
    Q_PROPERTY(bool     sendStatus  READ sendStatusText     WRITE setSendStatusText NOTIFY sendStatusChanged)

    // QML Access
    int     firmware        () { return (int)_firmwareType; }
    void    setFirmware     (int type) { _firmwareType = (MAV_AUTOPILOT)type; emit firmwareChanged(); }
    int     vehicle         () { return (int)_vehicleType; }
    void    setVehicle      (int type) { _vehicleType = (MAV_TYPE)type; emit vehicleChanged(); }

    MockConfiguration(const QString& name);
    MockConfiguration(MockConfiguration* source);

    MAV_AUTOPILOT firmwareType(void) { return _firmwareType; }
    void setFirmwareType(MAV_AUTOPILOT firmwareType) { _firmwareType = firmwareType; emit firmwareChanged(); }

    MAV_TYPE vehicleType(void) { return _vehicleType; }
    void setVehicleType(MAV_TYPE vehicleType) { _vehicleType = vehicleType; emit vehicleChanged(); }

    /// @param sendStatusText true: mavlink status text messages will be sent for each severity, as well as voice output info message
    bool sendStatusText(void) { return _sendStatusText; }
    void setSendStatusText(bool sendStatusText) { _sendStatusText = sendStatusText; emit sendStatusChanged(); }

    typedef enum {
        FailNone,                           // No failures
        FailParamNoReponseToRequestList,    // Do no respond to PARAM_REQUEST_LIST
        FailMissingParamOnInitialReqest,    // Not all params are sent on initial request, should still succeed since QGC will re-query missing params
        FailMissingParamOnAllRequests,      // Not all params are sent on initial request, QGC retries will fail as well
    } FailureMode_t;
    FailureMode_t failureMode(void) { return _failureMode; }
    void setFailureMode(FailureMode_t failureMode) { _failureMode = failureMode; }

    // Overrides from LinkConfiguration
    LinkType    type            (void) { return LinkConfiguration::TypeMock; }
    void        copyFrom        (LinkConfiguration* source);
    void        loadSettings    (QSettings& settings, const QString& root);
    void        saveSettings    (QSettings& settings, const QString& root);
    void        updateSettings  (void);
    QString     settingsURL     () { return "MockLinkSettings.qml"; }

signals:
    void firmwareChanged    ();
    void vehicleChanged     ();
    void sendStatusChanged  ();

private:
    MAV_AUTOPILOT   _firmwareType;
    MAV_TYPE        _vehicleType;
    bool            _sendStatusText;
    FailureMode_t   _failureMode;

    static const char* _firmwareTypeKey;
    static const char* _vehicleTypeKey;
    static const char* _sendStatusTextKey;
    static const char* _failureModeKey;
};

class MockLink : public LinkInterface
{
    Q_OBJECT

public:
    MockLink(SharedLinkConfigurationPointer& config);
    ~MockLink(void);

    // MockLink methods
    int vehicleId(void) { return _vehicleSystemId; }
    MAV_AUTOPILOT getFirmwareType(void) { return _firmwareType; }
    void setFirmwareType(MAV_AUTOPILOT autopilot) { _firmwareType = autopilot; }
    void setSendStatusText(bool sendStatusText) { _sendStatusText = sendStatusText; }
    void setFailureMode(MockConfiguration::FailureMode_t failureMode) { _failureMode = failureMode; }

    /// APM stack has strange handling of the first item of the mission list. If it has no
    /// onboard mission items, sometimes it sends back a home position in position 0 and
    /// sometimes it doesn't. Don't ask. This option allows you to configure that behavior
    /// for unit testing.
    void setAPMMissionResponseMode(bool sendHomePositionOnEmptyList) { _apmSendHomePositionOnEmptyList = sendHomePositionOnEmptyList; }

    void emitRemoteControlChannelRawChanged(int channel, uint16_t raw);

    /// Sends the specified mavlink message to QGC
    void respondWithMavlinkMessage(const mavlink_message_t& msg);

    MockLinkFileServer* getFileServer(void) { return _fileServer; }

    // Virtuals from LinkInterface
    virtual QString getName(void) const { return _name; }
    virtual void requestReset(void){ }
    virtual bool isConnected(void) const { return _connected; }
    virtual qint64 getConnectionSpeed(void) const { return 100000000; }
    virtual qint64 bytesAvailable(void) { return 0; }

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

    /// Sets a failure mode for unit testing
    ///     @param failureMode Type of failure to simulate
    void setMissionItemFailureMode(MockLinkMissionItemHandler::FailureMode_t failureMode);

    /// Called to send a MISSION_ACK message while the MissionManager is in idle state
    void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType) { _missionItemHandler.sendUnexpectedMissionAck(ackType); }

    /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
    void sendUnexpectedMissionItem(void) { _missionItemHandler.sendUnexpectedMissionItem(); }

    /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
    void sendUnexpectedMissionRequest(void) { _missionItemHandler.sendUnexpectedMissionRequest(); }

    /// Reset the state of the MissionItemHandler to no items, no transactions in progress.
    void resetMissionItemHandler(void) { _missionItemHandler.reset(); }

    /// Returns the filename for the simulated log file. Onyl available after a download is requested.
    QString logDownloadFile(void) { return _logDownloadFilename; }

    static MockLink* startPX4MockLink            (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startGenericMockLink        (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startAPMArduCopterMockLink  (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startAPMArduPlaneMockLink   (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    static MockLink* startAPMArduSubMockLink     (bool sendStatusText, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);

private slots:
    virtual void _writeBytes(const QByteArray bytes);

private slots:
    void _run1HzTasks(void);
    void _run10HzTasks(void);
    void _run500HzTasks(void);

private:
    // From LinkInterface
    virtual bool _connect(void);
    virtual void _disconnect(void);

    // QThread override
    virtual void run(void);

    // MockLink methods
    void _sendHeartBeat(void);
    void _handleIncomingNSHBytes(const char* bytes, int cBytes);
    void _handleIncomingMavlinkBytes(const uint8_t* bytes, int cBytes);
    void _loadParams(void);
    void _handleHeartBeat(const mavlink_message_t& msg);
    void _handleSetMode(const mavlink_message_t& msg);
    void _handleParamRequestList(const mavlink_message_t& msg);
    void _handleParamSet(const mavlink_message_t& msg);
    void _handleParamRequestRead(const mavlink_message_t& msg);
    void _handleFTP(const mavlink_message_t& msg);
    void _handleCommandLong(const mavlink_message_t& msg);
    void _handleManualControl(const mavlink_message_t& msg);
    void _handlePreFlightCalibration(const mavlink_command_long_t& request);
    void _handleLogRequestList(const mavlink_message_t& msg);
    void _handleLogRequestData(const mavlink_message_t& msg);
    float _floatUnionForParam(int componentId, const QString& paramName);
    void _setParamFloatUnionIntoMap(int componentId, const QString& paramName, float paramFloat);
    void _sendHomePosition(void);
    void _sendGpsRawInt(void);
    void _sendVibration(void);
    void _sendStatusTextMessages(void);
    void _respondWithAutopilotVersion(void);
    void _sendRCChannels(void);
    void _paramRequestListWorker(void);
    void _logDownloadWorker(void);

    static MockLink* _startMockLink(MockConfiguration* mockConfig);

    MockLinkMissionItemHandler  _missionItemHandler;

    QString _name;
    bool    _connected;

    uint8_t _vehicleSystemId;
    uint8_t _vehicleComponentId;

    bool    _inNSH;
    bool    _mavlinkStarted;

    QMap<int, QMap<QString, QVariant> > _mapParamName2Value;
    QMap<QString, MAV_PARAM_TYPE>       _mapParamName2MavParamType;

    uint8_t     _mavBaseMode;
    uint32_t    _mavCustomMode;
    uint8_t     _mavState;

    MAV_AUTOPILOT       _firmwareType;
    MAV_TYPE            _vehicleType;

    MockLinkFileServer* _fileServer;

    bool _sendStatusText;
    bool _apmSendHomePositionOnEmptyList;
    MockConfiguration::FailureMode_t _failureMode;

    int _sendHomePositionDelayCount;
    int _sendGPSPositionDelayCount;

    int _currentParamRequestListComponentIndex; // Current component index for param request list workflow, -1 for no request in progress
    int _currentParamRequestListParamIndex;     // Current parameter index for param request list workflow

    static const uint16_t _logDownloadLogId = 0;        ///< Id of siumulated log file
    static const uint32_t _logDownloadFileSize = 1000;  ///< Size of simulated log file

    QString _logDownloadFilename;           ///< Filename for log download which is in progress
    uint32_t    _logDownloadCurrentOffset;  ///< Current offset we are sending from
    uint32_t    _logDownloadBytesRemaining; ///< Number of bytes still to send, 0 = send inactive

    static float        _vehicleLatitude;
    static float        _vehicleLongitude;
    static float        _vehicleAltitude;
    static int          _nextVehicleSystemId;
    static const char*  _failParam;
};

#endif
