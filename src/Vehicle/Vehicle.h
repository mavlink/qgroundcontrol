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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef Vehicle_H
#define Vehicle_H

#include <QObject>
#include <QGeoCoordinate>

#include "LinkInterface.h"
#include "QGCMAVLink.h"
#include "MissionItem.h"
#include "QmlObjectListModel.h"
#include "MAVLinkProtocol.h"
#include "UASMessageHandler.h"

class UAS;
class UASInterface;
class FirmwarePlugin;
class FirmwarePluginManager;
class AutoPilotPlugin;
class AutoPilotPluginManager;
class MissionManager;
class ParameterLoader;
class JoystickManager;
class UASMessage;

Q_DECLARE_LOGGING_CATEGORY(VehicleLog)

class Vehicle : public QObject
{
    Q_OBJECT

public:
    Vehicle(LinkInterface*          link,
            int                     vehicleId,
            MAV_AUTOPILOT           firmwareType,
            MAV_TYPE                vehicleType,
            FirmwarePluginManager*  firmwarePluginManager,
            AutoPilotPluginManager* autopilotPluginManager,
            JoystickManager*        joystickManager);
    ~Vehicle();

    Q_PROPERTY(int                  id                      READ id                                     CONSTANT)
    Q_PROPERTY(AutoPilotPlugin*     autopilot               MEMBER _autopilotPlugin                     CONSTANT)
    Q_PROPERTY(QGeoCoordinate       coordinate              READ coordinate                             NOTIFY coordinateChanged)
    Q_PROPERTY(bool                 coordinateValid         READ coordinateValid                        NOTIFY coordinateValidChanged)
    Q_PROPERTY(MissionManager*      missionManager          MEMBER _missionManager                      CONSTANT)
    Q_PROPERTY(bool                 homePositionAvailable   READ homePositionAvailable                  NOTIFY homePositionAvailableChanged)
    Q_PROPERTY(QGeoCoordinate       homePosition            READ homePosition                           NOTIFY homePositionChanged)
    Q_PROPERTY(bool                 armed                   READ armed              WRITE setArmed      NOTIFY armedChanged)
    Q_PROPERTY(bool                 flightModeSetAvailable  READ flightModeSetAvailable                 CONSTANT)
    Q_PROPERTY(QStringList          flightModes             READ flightModes                            CONSTANT)
    Q_PROPERTY(QString              flightMode              READ flightMode         WRITE setFlightMode NOTIFY flightModeChanged)
    Q_PROPERTY(bool                 hilMode                 READ hilMode            WRITE setHilMode    NOTIFY hilModeChanged)
    Q_PROPERTY(bool                 missingParameters       READ missingParameters                      NOTIFY missingParametersChanged)
    Q_PROPERTY(QmlObjectListModel*  trajectoryPoints        READ trajectoryPoints                       CONSTANT)
    Q_PROPERTY(float                roll                    READ roll                                   NOTIFY rollChanged)
    Q_PROPERTY(float                pitch                   READ pitch                                  NOTIFY pitchChanged)
    Q_PROPERTY(float                heading                 READ heading                                NOTIFY headingChanged)
    Q_PROPERTY(float                groundSpeed             READ groundSpeed                            NOTIFY groundSpeedChanged)
    Q_PROPERTY(float                airSpeed                READ airSpeed                               NOTIFY airSpeedChanged)
    Q_PROPERTY(float                climbRate               READ climbRate                              NOTIFY climbRateChanged)
    Q_PROPERTY(float                altitudeRelative        READ altitudeRelative                       NOTIFY altitudeRelativeChanged)
    Q_PROPERTY(float                altitudeWGS84           READ altitudeWGS84                          NOTIFY altitudeWGS84Changed)
    Q_PROPERTY(float                altitudeAMSL            READ altitudeAMSL                           NOTIFY altitudeAMSLChanged)
    Q_PROPERTY(float                latitude                READ latitude                               NOTIFY coordinateChanged)
    Q_PROPERTY(float                longitude               READ longitude                              NOTIFY coordinateChanged)
    Q_PROPERTY(double               batteryVoltage          READ batteryVoltage                         NOTIFY batteryVoltageChanged)
    Q_PROPERTY(double               batteryPercent          READ batteryPercent                         NOTIFY batteryPercentChanged)
    Q_PROPERTY(double               batteryConsumed         READ batteryConsumed                        NOTIFY batteryConsumedChanged)
    Q_PROPERTY(int                  satelliteCount          READ satelliteCount                         NOTIFY satelliteCountChanged)
    Q_PROPERTY(double               satRawHDOP              READ satRawHDOP                             NOTIFY satRawHDOPChanged)
    Q_PROPERTY(double               satRawVDOP              READ satRawVDOP                             NOTIFY satRawVDOPChanged)
    Q_PROPERTY(double               satRawCOG               READ satRawCOG                              NOTIFY satRawCOGChanged)
    Q_PROPERTY(QString              currentState            READ currentState                           NOTIFY currentStateChanged)
    Q_PROPERTY(int                  satelliteLock           READ satelliteLock                          NOTIFY satelliteLockChanged)
    Q_PROPERTY(QmlObjectListModel*  missionItems            READ missionItemsModel                      CONSTANT)
    Q_PROPERTY(bool                 messageTypeNone         READ messageTypeNone                        NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeNormal       READ messageTypeNormal                      NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeWarning      READ messageTypeWarning                     NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeError        READ messageTypeError                       NOTIFY messageTypeChanged)
    Q_PROPERTY(int                  newMessageCount         READ newMessageCount                        NOTIFY newMessageCountChanged)
    Q_PROPERTY(int                  messageCount            READ messageCount                           NOTIFY messageCountChanged)
    Q_PROPERTY(QString              formatedMessages        READ formatedMessages                       NOTIFY formatedMessagesChanged)
    Q_PROPERTY(QString              formatedMessage         READ formatedMessage                        NOTIFY formatedMessageChanged)
    Q_PROPERTY(QString              latestError             READ latestError                            NOTIFY latestErrorChanged)
    Q_PROPERTY(int                  joystickMode            READ joystickMode       WRITE setJoystickMode NOTIFY joystickModeChanged)
    Q_PROPERTY(QStringList          joystickModes           READ joystickModes                          CONSTANT)
    Q_PROPERTY(bool                 joystickEnabled         READ joystickEnabled    WRITE setJoystickEnabled NOTIFY joystickEnabledChanged)
    Q_PROPERTY(bool                 active                  READ active             WRITE setActive     NOTIFY activeChanged)
    Q_PROPERTY(int                  flowImageIndex          READ flowImageIndex                         NOTIFY flowImageIndexChanged)
    Q_PROPERTY(int                  rcRSSI                  READ rcRSSI                                 NOTIFY rcRSSIChanged)
    Q_PROPERTY(bool                 px4Firmware             READ px4Firmware                            CONSTANT)
    Q_PROPERTY(bool                 apmFirmware             READ apmFirmware                            CONSTANT)
    Q_PROPERTY(bool                 genericFirmware         READ genericFirmware                        CONSTANT)
    Q_PROPERTY(bool                 connectionLost          READ connectionLost                         NOTIFY connectionLostChanged)
    Q_PROPERTY(bool                 connectionLostEnabled   READ connectionLostEnabled  WRITE setConnectionLostEnabled NOTIFY connectionLostEnabledChanged)

    /// Returns the number of buttons which are reserved for firmware use in the MANUAL_CONTROL mavlink
    /// message. For example PX4 Flight Stack reserves the first 8 buttons to simulate rc switches.
    /// The remainder can be assigned to Vehicle actions.
    /// @return -1: reserver all buttons, >0 number of buttons to reserve
    Q_PROPERTY(int manualControlReservedButtonCount READ manualControlReservedButtonCount CONSTANT)

    Q_INVOKABLE QString     getMavIconColor();

    // Called when the message drop-down is invoked to clear current count
    Q_INVOKABLE void        resetMessages();

    Q_INVOKABLE void virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust);
    Q_INVOKABLE void disconnectInactiveVehicle(void);

    // Property accessors

    QGeoCoordinate coordinate(void) { return _coordinate; }
    bool coordinateValid(void)      { return _coordinateValid; }
    QmlObjectListModel* missionItemsModel(void);

    typedef enum {
        JoystickModeRC,         ///< Joystick emulates an RC Transmitter
        JoystickModeAttitude,
        JoystickModePosition,
        JoystickModeForce,
        JoystickModeVelocity,
        JoystickModeMax
    } JoystickMode_t;

    int joystickMode(void);
    void setJoystickMode(int mode);

    /// List of joystick mode names
    QStringList joystickModes(void);

    bool joystickEnabled(void);
    void setJoystickEnabled(bool enabled);

    // Is vehicle active with respect to current active vehicle in QGC
    bool active(void);
    void setActive(bool active);

    // Property accesors
    int id(void) { return _id; }
    MAV_AUTOPILOT firmwareType(void) { return _firmwareType; }
    MAV_TYPE vehicleType(void) { return _vehicleType; }

    /// Returns the highest quality link available to the Vehicle
    LinkInterface* priorityLink(void);

    /// Sends a message to all links accociated with this vehicle
    void sendMessage(mavlink_message_t message);

    /// Sends a message to the specified link
    /// @return true: message sent, false: Link no longer connected
    bool sendMessageOnLink(LinkInterface* link, mavlink_message_t message);

    /// Sends the specified messages multiple times to the vehicle in order to attempt to
    /// guarantee that it makes it to the vehicle.
    void sendMessageMultiple(mavlink_message_t message);

    /// Provides access to uas from vehicle. Temporary workaround until UAS is fully phased out.
    UAS* uas(void) { return _uas; }

    /// Provides access to uas from vehicle. Temporary workaround until AutoPilotPlugin is fully phased out.
    AutoPilotPlugin* autopilotPlugin(void) { return _autopilotPlugin; }

    /// Provides access to the Firmware Plugin for this Vehicle
    FirmwarePlugin* firmwarePlugin(void) { return _firmwarePlugin; }

    int manualControlReservedButtonCount(void);

    MissionManager* missionManager(void) { return _missionManager; }

    bool homePositionAvailable(void);
    QGeoCoordinate homePosition(void);

    bool armed(void) { return _armed; }
    void setArmed(bool armed);

    bool flightModeSetAvailable(void);
    QStringList flightModes(void);
    QString flightMode(void);
    void setFlightMode(const QString& flightMode);

    bool hilMode(void);
    void setHilMode(bool hilMode);

    QmlObjectListModel* trajectoryPoints(void) { return &_mapTrajectoryList; }

    int  flowImageIndex() { return _flowImageIndex; }

    /// Requests the specified data stream from the vehicle
    ///     @param stream Stream which is being requested
    ///     @param rate Rate at which to send stream in Hz
    ///     @param sendMultiple Send multiple time to guarantee Vehicle reception
    void requestDataStream(MAV_DATA_STREAM stream, uint16_t rate, bool sendMultiple = true);

    bool missingParameters(void);

    typedef enum {
        MessageNone,
        MessageNormal,
        MessageWarning,
        MessageError
    } MessageType_t;

    enum {
        ROLL_CHANGED,
        PITCH_CHANGED,
        HEADING_CHANGED,
        GROUNDSPEED_CHANGED,
        AIRSPEED_CHANGED,
        CLIMBRATE_CHANGED,
        ALTITUDERELATIVE_CHANGED,
        ALTITUDEWGS84_CHANGED,
        ALTITUDEAMSL_CHANGED
    };

    bool            messageTypeNone     () { return _currentMessageType == MessageNone; }
    bool            messageTypeNormal   () { return _currentMessageType == MessageNormal; }
    bool            messageTypeWarning  () { return _currentMessageType == MessageWarning; }
    bool            messageTypeError    () { return _currentMessageType == MessageError; }
    int             newMessageCount     () { return _currentMessageCount; }
    int             messageCount        () { return _messageCount; }
    QString         formatedMessages    ();
    QString         formatedMessage     () { return _formatedMessage; }
    QString         latestError         () { return _latestError; }
    float           roll                () { return _roll; }
    float           pitch               () { return _pitch; }
    float           heading             () { return _heading; }
    float           groundSpeed         () { return _groundSpeed; }
    float           airSpeed            () { return _airSpeed; }
    float           climbRate           () { return _climbRate; }
    float           altitudeRelative    () { return _altitudeRelative; }
    float           altitudeWGS84       () { return _altitudeWGS84; }
    float           altitudeAMSL        () { return _altitudeAMSL; }
    float           latitude            () { return _coordinate.latitude(); }
    float           longitude           () { return _coordinate.longitude(); }
    bool            mavPresent          () { return _mav != NULL; }
    int             satelliteCount      () { return _satelliteCount; }
    double          satRawHDOP          () { return _satRawHDOP; }
    double          satRawVDOP          () { return _satRawVDOP; }
    double          satRawCOG           () { return _satRawCOG; }
    double          batteryVoltage      () { return _batteryVoltage; }
    double          batteryPercent      () { return _batteryPercent; }
    double          batteryConsumed     () { return _batteryConsumed; }
    QString         currentState        () { return _currentState; }
    int             satelliteLock       () { return _satelliteLock; }
    int             rcRSSI              () { return _rcRSSI; }
    bool            px4Firmware         () { return _firmwareType == MAV_AUTOPILOT_PX4; }
    bool            apmFirmware         () { return _firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA; }
    bool            genericFirmware     () { return !px4Firmware() && !apmFirmware(); }
    bool            connectionLost      () const { return _connectionLost; }
    bool            connectionLostEnabled() const { return _connectionLostEnabled; }

    void setConnectionLostEnabled(bool connectionLostEnabled);

    ParameterLoader* getParameterLoader(void);

    static const int cMaxRcChannels = 18;

    bool containsLink(LinkInterface* link) { return _links.contains(link); }

public slots:
    void setLatitude(double latitude);
    void setLongitude(double longitude);

signals:
    void allLinksInactive(Vehicle* vehicle);
    void coordinateChanged(QGeoCoordinate coordinate);
    void coordinateValidChanged(bool coordinateValid);
    void joystickModeChanged(int mode);
    void joystickEnabledChanged(bool enabled);
    void activeChanged(bool active);
    void mavlinkMessageReceived(const mavlink_message_t& message);
    void homePositionAvailableChanged(bool homePositionAvailable);
    void homePositionChanged(const QGeoCoordinate& homePosition);
    void armedChanged(bool armed);
    void flightModeChanged(const QString& flightMode);
    void hilModeChanged(bool hilMode);
    void missingParametersChanged(bool missingParameters);
    void connectionLostChanged(bool connectionLost);
    void connectionLostEnabledChanged(bool connectionLostEnabled);

    /// Used internally to move sendMessage call to main thread
    void _sendMessageOnThread(mavlink_message_t message);
    void _sendMessageOnLinkOnThread(LinkInterface* link, mavlink_message_t message);

    void messageTypeChanged     ();
    void newMessageCountChanged ();
    void messageCountChanged    ();
    void formatedMessagesChanged();
    void formatedMessageChanged ();
    void latestErrorChanged     ();
    void rollChanged            ();
    void pitchChanged           ();
    void headingChanged         ();
    void groundSpeedChanged     ();
    void airSpeedChanged        ();
    void climbRateChanged       ();
    void altitudeRelativeChanged();
    void altitudeWGS84Changed   ();
    void altitudeAMSLChanged    ();
    void longitudeChanged       ();
    void batteryVoltageChanged  ();
    void batteryPercentChanged  ();
    void batteryConsumedChanged ();
    void currentConfigChanged   ();
    void satelliteCountChanged  ();
    void satRawHDOPChanged      ();
    void satRawVDOPChanged      ();
    void satRawCOGChanged       ();
    void currentStateChanged    ();
    void satelliteLockChanged   ();
    void flowImageIndexChanged  ();
    void rcRSSIChanged          (int rcRSSI);

    /// New RC channel values
    ///     @param channelCount Number of available channels, cMaxRcChannels max
    ///     @param pwmValues -1 signals channel not available
    void rcChannelsChanged(int channelCount, int pwmValues[cMaxRcChannels]);

    /// Remote control RSSI changed  (0% - 100%)
    void remoteControlRSSIChanged(uint8_t rssi);

    void mavlinkRawImu(mavlink_message_t message);
    void mavlinkScaledImu1(mavlink_message_t message);
    void mavlinkScaledImu2(mavlink_message_t message);
    void mavlinkScaledImu3(mavlink_message_t message);

private slots:
    void _mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message);
    void _linkInactiveOrDeleted(LinkInterface* link);
    void _sendMessage(mavlink_message_t message);
    void _sendMessageOnLink(LinkInterface* link, mavlink_message_t message);
    void _sendMessageMultipleNext(void);
    void _addNewMapTrajectoryPoint(void);
    void _parametersReady(bool parametersReady);
    void _remoteControlRSSIChanged(uint8_t rssi);

    void _handleTextMessage                 (int newCount);
    void _handletextMessageReceived         (UASMessage* message);
    /** @brief Attitude from main autopilot / system state */
    void _updateAttitude                    (UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp);
    /** @brief Attitude from one specific component / redundant autopilot */
    void _updateAttitude                    (UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 timestamp);
    void _updateSpeed                       (UASInterface* uas, double _groundSpeed, double _airSpeed, quint64 timestamp);
    void _updateAltitude                    (UASInterface* uas, double _altitudeAMSL, double _altitudeWGS84, double _altitudeRelative, double _climbRate, quint64 timestamp);
    void _updateNavigationControllerErrors  (UASInterface* uas, double altitudeError, double speedError, double xtrackError);
    void _updateNavigationControllerData    (UASInterface *uas, float navRoll, float navPitch, float navBearing, float targetBearing, float targetDistance);
    void _checkUpdate                       ();
    void _updateBatteryRemaining            (UASInterface*, double voltage, double, double percent, int);
    void _updateBatteryConsumedChanged      (UASInterface*, double current_consumed);
    void _updateState                       (UASInterface* system, QString name, QString description);
    void _setSatelliteCount                 (double val, QString name);
    void _setSatRawHDOP                     (double val);
    void _setSatRawVDOP                     (double val);
    void _setSatRawCOG                      (double val);
    void _setSatLoc                         (UASInterface* uas, int fix);
    /** @brief A new camera image has arrived */
    void _imageReady                        (UASInterface* uas);
    void _connectionLostTimeout(void);

private:
    bool _containsLink(LinkInterface* link);
    void _addLink(LinkInterface* link);
    void _loadSettings(void);
    void _saveSettings(void);
    void _startJoystick(bool start);
    void _handleHomePosition(mavlink_message_t& message);
    void _handleHeartbeat(mavlink_message_t& message);
    void _handleRCChannels(mavlink_message_t& message);
    void _handleRCChannelsRaw(mavlink_message_t& message);
    void _missionManagerError(int errorCode, const QString& errorMsg);
    void _mapTrajectoryStart(void);
    void _mapTrajectoryStop(void);
    void _connectionActive(void);
    void _say(const QString& text, int severity);

    void    _addChange                      (int id);
    float   _oneDecimal                     (float value);

private:
    int     _id;            ///< Mavlink system id
    bool    _active;

    MAV_AUTOPILOT       _firmwareType;
    MAV_TYPE            _vehicleType;
    FirmwarePlugin*     _firmwarePlugin;
    AutoPilotPlugin*    _autopilotPlugin;
    MAVLinkProtocol*    _mavlink;

    QList<LinkInterface*> _links;

    JoystickMode_t  _joystickMode;
    bool            _joystickEnabled;

    UAS* _uas;

    QGeoCoordinate  _coordinate;
    bool            _coordinateValid;       ///< true: vehicle has 3d lock and therefore valid location

    bool            _homePositionAvailable;
    QGeoCoordinate  _homePosition;

    UASInterface*   _mav;
    int             _currentMessageCount;
    int             _messageCount;
    int             _currentErrorCount;
    int             _currentWarningCount;
    int             _currentNormalCount;
    MessageType_t   _currentMessageType;
    QString         _latestError;
    float           _roll;
    float           _pitch;
    float           _heading;
    float           _altitudeAMSL;
    float           _altitudeWGS84;
    float           _altitudeRelative;
    float           _groundSpeed;
    float           _airSpeed;
    float           _climbRate;
    float           _navigationAltitudeError;
    float           _navigationSpeedError;
    float           _navigationCrosstrackError;
    float           _navigationTargetBearing;
    QTimer*         _refreshTimer;
    QList<int>      _changes;
    double          _batteryVoltage;
    double          _batteryPercent;
    double          _batteryConsumed;
    QString         _currentState;
    int             _satelliteCount;
    double          _satRawHDOP;
    double          _satRawVDOP;
    double          _satRawCOG;
    int             _satelliteLock;
    int             _updateCount;
    QString         _formatedMessage;
    int             _rcRSSI;
    double          _rcRSSIstore;

    // Lost connection handling
    bool                _connectionLost;
    bool                _connectionLostEnabled;
    static const int    _connectionLostTimeoutMSecs = 3500;  // Signal connection lost after 3.5 seconds of missed heartbeat
    QTimer              _connectionLostTimer;

    MissionManager*     _missionManager;
    bool                _missionManagerInitialRequestComplete;

    ParameterLoader*    _parameterLoader;

    bool    _armed;         ///< true: vehicle is armed
    uint8_t _base_mode;     ///< base_mode from HEARTBEAT
    uint32_t _custom_mode;  ///< custom_mode from HEARTBEAT

    /// Used to store a message being sent by sendMessageMultiple
    typedef struct {
        mavlink_message_t   message;    ///< Message to send multiple times
        int                 retryCount; ///< Number of retries left
    } SendMessageMultipleInfo_t;

    QList<SendMessageMultipleInfo_t> _sendMessageMultipleList;    ///< List of messages being sent multiple times

    static const int _sendMessageMultipleRetries = 5;
    static const int _sendMessageMultipleIntraMessageDelay = 500;

    QTimer  _sendMultipleTimer;
    int     _nextSendMessageMultipleIndex;

    QTimer              _mapTrajectoryTimer;
    QmlObjectListModel  _mapTrajectoryList;
    QGeoCoordinate      _mapTrajectoryLastCoordinate;
    bool                _mapTrajectoryHaveFirstCoordinate;
    static const int    _mapTrajectoryMsecsBetweenPoints = 1000;

    FirmwarePluginManager*      _firmwarePluginManager;
    AutoPilotPluginManager*     _autopilotPluginManager;
    JoystickManager*            _joystickManager;

    int                         _flowImageIndex;

    bool _allLinksInactiveSent; ///< true: allLinkInactive signal already sent one time

    // Settings keys
    static const char* _settingsGroup;
    static const char* _joystickModeSettingsKey;
    static const char* _joystickEnabledSettingsKey;
};
#endif
