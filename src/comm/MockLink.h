/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

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

    static const char* _firmwareTypeKey;
    static const char* _vehicleTypeKey;
    static const char* _sendStatusTextKey;
};

class MockLink : public LinkInterface
{
    Q_OBJECT

public:
    // LinkConfiguration is optional for MockLink
    MockLink(MockConfiguration* config = NULL);
    ~MockLink(void);

    // MockLink methods
    int vehicleId(void) { return _vehicleSystemId; }
    MAV_AUTOPILOT getFirmwareType(void) { return _firmwareType; }
    void setFirmwareType(MAV_AUTOPILOT autopilot) { _firmwareType = autopilot; }
    void setSendStatusText(bool sendStatusText) { _sendStatusText = sendStatusText; }

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

    LinkConfiguration* getLinkConfiguration() { return _config; }

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

    static MockLink* startPX4MockLink            (bool sendStatusText);
    static MockLink* startGenericMockLink        (bool sendStatusText);
    static MockLink* startAPMArduCopterMockLink  (bool sendStatusText);
    static MockLink* startAPMArduPlaneMockLink   (bool sendStatusText);

private slots:
    virtual void _writeBytes(const QByteArray bytes);

private slots:
    void _run1HzTasks(void);
    void _run10HzTasks(void);
    void _run50HzTasks(void);

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
    float _floatUnionForParam(int componentId, const QString& paramName);
    void _setParamFloatUnionIntoMap(int componentId, const QString& paramName, float paramFloat);
    void _sendHomePosition(void);
    void _sendGpsRawInt(void);
    void _sendVibration(void);
    void _sendStatusTextMessages(void);

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

    MockConfiguration*  _config;
    MAV_AUTOPILOT       _firmwareType;
    MAV_TYPE            _vehicleType;

    MockLinkFileServer* _fileServer;

    bool _sendStatusText;
    bool _apmSendHomePositionOnEmptyList;

    int _sendHomePositionDelayCount;

    static float _vehicleLatitude;
    static float _vehicleLongitude;
    static float _vehicleAltitude;
};

#endif
