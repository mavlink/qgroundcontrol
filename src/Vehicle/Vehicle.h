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
#include <QElapsedTimer>

#include "FactGroup.h"
#include "LinkInterface.h"
#include "QGCMAVLink.h"
#include "QmlObjectListModel.h"
#include "MAVLinkProtocol.h"
#include "UASMessageHandler.h"
#include "SettingsFact.h"

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

class Vehicle;

class VehicleVibrationFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleVibrationFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* xAxis      READ xAxis      CONSTANT)
    Q_PROPERTY(Fact* yAxis      READ yAxis      CONSTANT)
    Q_PROPERTY(Fact* zAxis      READ zAxis      CONSTANT)
    Q_PROPERTY(Fact* clipCount1 READ clipCount1 CONSTANT)
    Q_PROPERTY(Fact* clipCount2 READ clipCount2 CONSTANT)
    Q_PROPERTY(Fact* clipCount3 READ clipCount3 CONSTANT)

    Fact* xAxis         (void) { return &_xAxisFact; }
    Fact* yAxis         (void) { return &_yAxisFact; }
    Fact* zAxis         (void) { return &_zAxisFact; }
    Fact* clipCount1    (void) { return &_clipCount1Fact; }
    Fact* clipCount2    (void) { return &_clipCount2Fact; }
    Fact* clipCount3    (void) { return &_clipCount3Fact; }

    void setVehicle(Vehicle* vehicle);

    static const char* _xAxisFactName;
    static const char* _yAxisFactName;
    static const char* _zAxisFactName;
    static const char* _clipCount1FactName;
    static const char* _clipCount2FactName;
    static const char* _clipCount3FactName;

private:
    Vehicle*    _vehicle;
    Fact        _xAxisFact;
    Fact        _yAxisFact;
    Fact        _zAxisFact;
    Fact        _clipCount1Fact;
    Fact        _clipCount2Fact;
    Fact        _clipCount3Fact;
};

class VehicleWindFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleWindFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* direction      READ direction      CONSTANT)
    Q_PROPERTY(Fact* speed          READ speed          CONSTANT)
    Q_PROPERTY(Fact* verticalSpeed  READ verticalSpeed  CONSTANT)

    Fact* direction     (void) { return &_directionFact; }
    Fact* speed         (void) { return &_speedFact; }
    Fact* verticalSpeed (void) { return &_verticalSpeedFact; }

    void setVehicle(Vehicle* vehicle);

    static const char* _directionFactName;
    static const char* _speedFactName;
    static const char* _verticalSpeedFactName;

private:
    Vehicle*    _vehicle;
    Fact        _directionFact;
    Fact        _speedFact;
    Fact        _verticalSpeedFact;
};

class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleGPSFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* hdop               READ hdop               CONSTANT)
    Q_PROPERTY(Fact* vdop               READ vdop               CONSTANT)
    Q_PROPERTY(Fact* courseOverGround   READ courseOverGround   CONSTANT)
    Q_PROPERTY(Fact* count              READ count              CONSTANT)
    Q_PROPERTY(Fact* lock               READ lock               CONSTANT)

    Fact* hdop              (void) { return &_hdopFact; }
    Fact* vdop              (void) { return &_vdopFact; }
    Fact* courseOverGround  (void) { return &_courseOverGroundFact; }
    Fact* count             (void) { return &_countFact; }
    Fact* lock              (void) { return &_lockFact; }

    void setVehicle(Vehicle* vehicle);

    static const char* _hdopFactName;
    static const char* _vdopFactName;
    static const char* _courseOverGroundFactName;
    static const char* _countFactName;
    static const char* _lockFactName;

private slots:
    void _setSatelliteCount(double val, QString);
    void _setSatRawHDOP(double val);
    void _setSatRawVDOP(double val);
    void _setSatRawCOG(double val);
    void _setSatLoc(UASInterface*, int fix);

private:
    Vehicle*    _vehicle;
    Fact        _hdopFact;
    Fact        _vdopFact;
    Fact        _courseOverGroundFact;
    Fact        _countFact;
    Fact        _lockFact;
};

class VehicleBatteryFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleBatteryFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* voltage            READ voltage            CONSTANT)
    Q_PROPERTY(Fact* percentRemaining   READ percentRemaining   CONSTANT)
    Q_PROPERTY(Fact* mahConsumed        READ mahConsumed        CONSTANT)
    Q_PROPERTY(Fact* current            READ current            CONSTANT)
    Q_PROPERTY(Fact* temperature        READ temperature        CONSTANT)
    Q_PROPERTY(Fact* cellCount          READ cellCount          CONSTANT)

    /// If percentRemaining falls below this value, warning will be output through speech
    Q_PROPERTY(Fact* percentRemainingAnnounce READ percentRemainingAnnounce CONSTANT)

    Fact* voltage                   (void) { return &_voltageFact; }
    Fact* percentRemaining          (void) { return &_percentRemainingFact; }
    Fact* percentRemainingAnnounce  (void);
    Fact* mahConsumed               (void) { return &_mahConsumedFact; }
    Fact* current                   (void) { return &_currentFact; }
    Fact* temperature               (void) { return &_temperatureFact; }
    Fact* cellCount                 (void) { return &_cellCountFact; }


    void setVehicle(Vehicle* vehicle);

    static const char* _voltageFactName;
    static const char* _percentRemainingFactName;
    static const char* _percentRemainingAnnounceFactName;
    static const char* _mahConsumedFactName;
    static const char* _currentFactName;
    static const char* _temperatureFactName;
    static const char* _cellCountFactName;

    static const char* _settingsGroup;
    static const int   _percentRemainingAnnounceDefault;

    static const double _voltageUnavailable;
    static const int    _percentRemainingUnavailable;
    static const int    _mahConsumedUnavailable;
    static const int    _currentUnavailable;
    static const double _temperatureUnavailable;
    static const int    _cellCountUnavailable;

private:
    Vehicle*        _vehicle;
    Fact            _voltageFact;
    Fact            _percentRemainingFact;
    Fact            _mahConsumedFact;
    Fact            _currentFact;
    Fact            _temperatureFact;
    Fact            _cellCountFact;

    /// This fact is global to all Vehicles. We must allocated the first time we need it so we don't
    /// run into QSettings application setup ordering issues.
    static SettingsFact* _percentRemainingAnnounceFact;
};

class Vehicle : public FactGroup
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

    // The following is used to create a disconnected Vehicle. A disconnected vehicle is primarily used to access FactGroup information
    // without needing a real connection.
    Vehicle(QObject* parent = NULL);

    ~Vehicle();

    Q_PROPERTY(int                  id                      READ id                                     CONSTANT)
    Q_PROPERTY(AutoPilotPlugin*     autopilot               MEMBER _autopilotPlugin                     CONSTANT)
    Q_PROPERTY(QGeoCoordinate       coordinate              READ coordinate                             NOTIFY coordinateChanged)
    Q_PROPERTY(bool                 coordinateValid         READ coordinateValid                        NOTIFY coordinateValidChanged)
    Q_PROPERTY(bool                 homePositionAvailable   READ homePositionAvailable                  NOTIFY homePositionAvailableChanged)
    Q_PROPERTY(QGeoCoordinate       homePosition            READ homePosition                           NOTIFY homePositionChanged)
    Q_PROPERTY(bool                 armed                   READ armed              WRITE setArmed      NOTIFY armedChanged)
    Q_PROPERTY(bool                 flightModeSetAvailable  READ flightModeSetAvailable                 CONSTANT)
    Q_PROPERTY(QStringList          flightModes             READ flightModes                            CONSTANT)
    Q_PROPERTY(QString              flightMode              READ flightMode         WRITE setFlightMode NOTIFY flightModeChanged)
    Q_PROPERTY(bool                 hilMode                 READ hilMode            WRITE setHilMode    NOTIFY hilModeChanged)
    Q_PROPERTY(bool                 missingParameters       READ missingParameters                      NOTIFY missingParametersChanged)
    Q_PROPERTY(QmlObjectListModel*  trajectoryPoints        READ trajectoryPoints                       CONSTANT)
    Q_PROPERTY(float                latitude                READ latitude                               NOTIFY coordinateChanged)
    Q_PROPERTY(float                longitude               READ longitude                              NOTIFY coordinateChanged)
    Q_PROPERTY(QString              currentState            READ currentState                           NOTIFY currentStateChanged)
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
    Q_PROPERTY(uint                 messagesReceived        READ messagesReceived                       NOTIFY messagesReceivedChanged)
    Q_PROPERTY(uint                 messagesSent            READ messagesSent                           NOTIFY messagesSentChanged)
    Q_PROPERTY(uint                 messagesLost            READ messagesLost                           NOTIFY messagesLostChanged)
    Q_PROPERTY(bool                 fixedWing               READ fixedWing                              CONSTANT)
    Q_PROPERTY(bool                 multiRotor              READ multiRotor                             CONSTANT)
    Q_PROPERTY(bool                 autoDisconnect          MEMBER _autoDisconnect                      NOTIFY autoDisconnectChanged)
    Q_PROPERTY(QString              prearmError             READ prearmError        WRITE setPrearmError NOTIFY prearmErrorChanged)

    /// true: Vehicle is flying, false: Vehicle is on ground
    Q_PROPERTY(bool flying      READ flying     WRITE setFlying     NOTIFY flyingChanged)

    /// true: Vehicle is in Guided mode and can respond to guided commands, false: vehicle cannot respond to direct control commands
    Q_PROPERTY(bool guidedMode  READ guidedMode WRITE setGuidedMode NOTIFY guidedModeChanged)

    /// true: Guided mode commands are supported by this vehicle
    Q_PROPERTY(bool guidedModeSupported READ guidedModeSupported CONSTANT)

    /// true: pauseVehicle command is supported
    Q_PROPERTY(bool pauseVehicleSupported READ pauseVehicleSupported CONSTANT)

    // FactGroup object model properties

    Q_PROPERTY(Fact* roll               READ roll               CONSTANT)
    Q_PROPERTY(Fact* pitch              READ pitch              CONSTANT)
    Q_PROPERTY(Fact* heading            READ heading            CONSTANT)
    Q_PROPERTY(Fact* groundSpeed        READ groundSpeed        CONSTANT)
    Q_PROPERTY(Fact* airSpeed           READ airSpeed           CONSTANT)
    Q_PROPERTY(Fact* climbRate          READ climbRate          CONSTANT)
    Q_PROPERTY(Fact* altitudeRelative   READ altitudeRelative   CONSTANT)
    Q_PROPERTY(Fact* altitudeAMSL       READ altitudeAMSL       CONSTANT)

    Q_PROPERTY(FactGroup* gps       READ gpsFactGroup       CONSTANT)
    Q_PROPERTY(FactGroup* battery   READ batteryFactGroup   CONSTANT)
    Q_PROPERTY(FactGroup* wind      READ windFactGroup      CONSTANT)
    Q_PROPERTY(FactGroup* vibration READ vibrationFactGroup CONSTANT)

    /// Resets link status counters
    Q_INVOKABLE void resetCounters  ();

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

    Q_INVOKABLE void clearTrajectoryPoints(void);

    /// Command vehicle to return to launch
    Q_INVOKABLE void guidedModeRTL(void);

    /// Command vehicle to land at current location
    Q_INVOKABLE void guidedModeLand(void);

    /// Command vehicle to takeoff from current location
    Q_INVOKABLE void guidedModeTakeoff(double altitudeRel);

    /// Command vehicle to move to specified location (altitude is included and relative)
    Q_INVOKABLE void guidedModeGotoLocation(const QGeoCoordinate& gotoCoord);

    /// Command vehicle to change to the specified relatice altitude
    Q_INVOKABLE void guidedModeChangeAltitude(double altitudeRel);

    /// Command vehicle to pause at current location. If vehicle supports guide mode, vehicle will be left
    /// in guided mode after pause.
    Q_INVOKABLE void pauseVehicle(void);

    /// Command vehicle to kill all motors no matter what state
    Q_INVOKABLE void emergencyStop(void);

    /// Alter the current mission item on the vehicle
    Q_INVOKABLE void setCurrentMissionSequence(int seq);

    bool guidedModeSupported(void) const;
    bool pauseVehicleSupported(void) const;

    // Property accessors

    QGeoCoordinate coordinate(void) { return _coordinate; }
    bool coordinateValid(void)      { return _coordinateValid; }
    void _setCoordinateValid(bool coordinateValid);

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
    MAV_AUTOPILOT firmwareType(void) const { return _firmwareType; }
    MAV_TYPE vehicleType(void) const { return _vehicleType; }

    /// Returns the highest quality link available to the Vehicle
    LinkInterface* priorityLink(void);

    /// Sends a message to all links accociated with this vehicle
    void sendMessage(mavlink_message_t message);

    /// Sends a message to the specified link
    /// @return true: message sent, false: Link no longer connected
    bool sendMessageOnLink(LinkInterface* link, mavlink_message_t message);

    /// Sends a message to the priority link
    /// @return true: message sent, false: Link no longer connected
    bool sendMessageOnPriorityLink(mavlink_message_t message) { return sendMessageOnLink(priorityLink(), message); }

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
    QString flightMode(void) const;
    void setFlightMode(const QString& flightMode);


    bool hilMode(void);
    void setHilMode(bool hilMode);

    bool fixedWing(void) const;
    bool multiRotor(void) const;

    void setFlying(bool flying);
    void setGuidedMode(bool guidedMode);

    QString prearmError(void) const { return _prearmError; }
    void setPrearmError(const QString& prearmError);

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

    bool            messageTypeNone     () { return _currentMessageType == MessageNone; }
    bool            messageTypeNormal   () { return _currentMessageType == MessageNormal; }
    bool            messageTypeWarning  () { return _currentMessageType == MessageWarning; }
    bool            messageTypeError    () { return _currentMessageType == MessageError; }
    int             newMessageCount     () { return _currentMessageCount; }
    int             messageCount        () { return _messageCount; }
    QString         formatedMessages    ();
    QString         formatedMessage     () { return _formatedMessage; }
    QString         latestError         () { return _latestError; }
    float           latitude            () { return _coordinate.latitude(); }
    float           longitude           () { return _coordinate.longitude(); }
    bool            mavPresent          () { return _mav != NULL; }
    QString         currentState        () { return _currentState; }
    int             rcRSSI              () { return _rcRSSI; }
    bool            px4Firmware         () const { return _firmwareType == MAV_AUTOPILOT_PX4; }
    bool            apmFirmware         () const { return _firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA; }
    bool            genericFirmware     () const { return !px4Firmware() && !apmFirmware(); }
    bool            connectionLost      () const { return _connectionLost; }
    bool            connectionLostEnabled() const { return _connectionLostEnabled; }
    uint            messagesReceived    () { return _messagesReceived; }
    uint            messagesSent        () { return _messagesSent; }
    uint            messagesLost        () { return _messagesLost; }
    bool            flying              () const { return _flying; }
    bool            guidedMode          () const;
    uint8_t         baseMode            () const { return _base_mode; }
    uint32_t        customMode          () const { return _custom_mode; }

    Fact* roll              (void) { return &_rollFact; }
    Fact* heading           (void) { return &_headingFact; }
    Fact* pitch             (void) { return &_pitchFact; }
    Fact* airSpeed          (void) { return &_airSpeedFact; }
    Fact* groundSpeed       (void) { return &_groundSpeedFact; }
    Fact* climbRate         (void) { return &_climbRateFact; }
    Fact* altitudeRelative  (void) { return &_altitudeRelativeFact; }
    Fact* altitudeAMSL      (void) { return &_altitudeAMSLFact; }

    FactGroup* gpsFactGroup         (void) { return &_gpsFactGroup; }
    FactGroup* batteryFactGroup     (void) { return &_batteryFactGroup; }
    FactGroup* windFactGroup        (void) { return &_windFactGroup; }
    FactGroup* vibrationFactGroup   (void) { return &_vibrationFactGroup; }

    void setConnectionLostEnabled(bool connectionLostEnabled);

    ParameterLoader* getParameterLoader(void);

    static const int cMaxRcChannels = 18;

    bool containsLink(LinkInterface* link) { return _links.contains(link); }
    void doCommandLong(int component, MAV_CMD command, float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);

    int firmwareMajorVersion(void) const { return _firmwareMajorVersion; }
    int firmwareMinorVersion(void) const { return _firmwareMinorVersion; }
    int firmwarePatchVersion(void) const { return _firmwarePatchVersion; }
    void setFirmwareVersion(int majorVersion, int minorVersion, int patchVersion);
    static const int versionNotSetValue = -1;

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
    void autoDisconnectChanged(bool autoDisconnectChanged);
    void flyingChanged(bool flying);
    void guidedModeChanged(bool guidedMode);
    void prearmErrorChanged(const QString& prearmError);
    void commandLongAck(uint8_t compID, uint16_t command, uint8_t result);

    void messagesReceivedChanged    ();
    void messagesSentChanged        ();
    void messagesLostChanged        ();

    /// Used internally to move sendMessage call to main thread
    void _sendMessageOnThread(mavlink_message_t message);
    void _sendMessageOnLinkOnThread(LinkInterface* link, mavlink_message_t message);

    void messageTypeChanged     ();
    void newMessageCountChanged ();
    void messageCountChanged    ();
    void formatedMessagesChanged();
    void formatedMessageChanged ();
    void latestErrorChanged     ();
    void longitudeChanged       ();
    void currentConfigChanged   ();
    void currentStateChanged    ();
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
    void _handleFlightModeChanged(const QString& flightMode);
    void _announceArmedChanged(bool armed);

    void _handleTextMessage                 (int newCount);
    void _handletextMessageReceived         (UASMessage* message);
    /** @brief Attitude from main autopilot / system state */
    void _updateAttitude                    (UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp);
    /** @brief Attitude from one specific component / redundant autopilot */
    void _updateAttitude                    (UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 timestamp);
    void _updateSpeed                       (UASInterface* uas, double _groundSpeed, double _airSpeed, quint64 timestamp);
    void _updateAltitude                    (UASInterface* uas, double _altitudeAMSL, double _altitudeRelative, double _climbRate, quint64 timestamp);
    void _updateNavigationControllerErrors  (UASInterface* uas, double altitudeError, double speedError, double xtrackError);
    void _updateNavigationControllerData    (UASInterface *uas, float navRoll, float navPitch, float navBearing, float targetBearing, float targetDistance);
    void _checkUpdate                       ();
    void _updateState                       (UASInterface* system, QString name, QString description);
    /** @brief A new camera image has arrived */
    void _imageReady                        (UASInterface* uas);
    void _connectionLostTimeout(void);
    void _prearmErrorTimeout(void);

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
    void _handleBatteryStatus(mavlink_message_t& message);
    void _handleSysStatus(mavlink_message_t& message);
    void _handleWind(mavlink_message_t& message);
    void _handleVibration(mavlink_message_t& message);
    void _handleExtendedSysState(mavlink_message_t& message);
    void _handleCommandAck(mavlink_message_t& message);
    void _missionManagerError(int errorCode, const QString& errorMsg);
    void _mapTrajectoryStart(void);
    void _mapTrajectoryStop(void);
    void _connectionActive(void);
    void _say(const QString& text);
    QString _vehicleIdSpeech(void);

private:
    int     _id;            ///< Mavlink system id
    bool    _active;
    bool    _disconnectedVehicle;   ///< This Vehicle is a "disconnected" vehicle for ui use when no active vehicle is available

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
    float           _navigationAltitudeError;
    float           _navigationSpeedError;
    float           _navigationCrosstrackError;
    float           _navigationTargetBearing;
    QTimer*         _refreshTimer;
    QString         _currentState;
    int             _updateCount;
    QString         _formatedMessage;
    int             _rcRSSI;
    double          _rcRSSIstore;
    bool            _autoDisconnect;    ///< true: Automatically disconnect vehicle when last connection goes away or lost heartbeat
    bool            _flying;

    QString             _prearmError;
    QTimer              _prearmErrorTimer;
    static const int    _prearmErrorTimeoutMSecs = 35 * 1000;   ///< Take away prearm error after 35 seconds

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

    // Toolbox references
    FirmwarePluginManager*      _firmwarePluginManager;
    AutoPilotPluginManager*     _autopilotPluginManager;
    JoystickManager*            _joystickManager;

    int                         _flowImageIndex;

    bool _allLinksInactiveSent; ///< true: allLinkInactive signal already sent one time

    uint                _messagesReceived;
    uint                _messagesSent;
    uint                _messagesLost;
    uint8_t             _messageSeq;
    uint8_t             _compID;
    bool                _heardFrom;

    int _firmwareMajorVersion;
    int _firmwareMinorVersion;
    int _firmwarePatchVersion;

    static const int    _lowBatteryAnnounceRepeatMSecs; // Amount of time in between each low battery announcement
    QElapsedTimer       _lowBatteryAnnounceTimer;

    // FactGroup facts

    Fact _rollFact;
    Fact _pitchFact;
    Fact _headingFact;
    Fact _groundSpeedFact;
    Fact _airSpeedFact;
    Fact _climbRateFact;
    Fact _altitudeRelativeFact;
    Fact _altitudeAMSLFact;

    VehicleGPSFactGroup         _gpsFactGroup;
    VehicleBatteryFactGroup     _batteryFactGroup;
    VehicleWindFactGroup        _windFactGroup;
    VehicleVibrationFactGroup   _vibrationFactGroup;

    static const char* _rollFactName;
    static const char* _pitchFactName;
    static const char* _headingFactName;
    static const char* _groundSpeedFactName;
    static const char* _airSpeedFactName;
    static const char* _climbRateFactName;
    static const char* _altitudeRelativeFactName;
    static const char* _altitudeAMSLFactName;

    static const char* _gpsFactGroupName;
    static const char* _batteryFactGroupName;
    static const char* _windFactGroupName;
    static const char* _vibrationFactGroupName;

    static const int _vehicleUIUpdateRateMSecs = 100;

    // Settings keys
    static const char* _settingsGroup;
    static const char* _joystickModeSettingsKey;
    static const char* _joystickEnabledSettingsKey;

};
#endif
