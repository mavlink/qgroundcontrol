/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QTime>
#include <QDateTime>
#include <QLocale>
#include <QQuaternion>

#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "FirmwarePluginManager.h"
#include "LinkManager.h"
#include "FirmwarePlugin.h"
#include "UAS.h"
#include "JoystickManager.h"
#include "MissionManager.h"
#include "MissionController.h"
#include "PlanMasterController.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"
#include "CoordinateVector.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCImageProvider.h"
#include "AudioOutput.h"
#include "FollowMe.h"
#include "MissionCommandTree.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"
#include "QGCQGeoCoordinate.h"
#include "QGCCorePlugin.h"
#include "ADSBVehicle.h"
#include "QGCCameraManager.h"
#include "VideoReceiver.h"
#include "VideoManager.h"

QGC_LOGGING_CATEGORY(VehicleLog, "VehicleLog")

#define UPDATE_TIMER 50
#define DEFAULT_LAT  38.965767f
#define DEFAULT_LON -120.083923f

const QString guided_mode_not_supported_by_vehicle = QObject::tr("Guided mode not supported by Vehicle.");

const char* Vehicle::_settingsGroup =               "Vehicle%1";        // %1 replaced with mavlink system id
const char* Vehicle::_joystickModeSettingsKey =     "JoystickMode";
const char* Vehicle::_joystickEnabledSettingsKey =  "JoystickEnabled";

const char* Vehicle::_rollFactName =                "roll";
const char* Vehicle::_pitchFactName =               "pitch";
const char* Vehicle::_headingFactName =             "heading";
const char* Vehicle::_rollRateFactName =             "rollRate";
const char* Vehicle::_pitchRateFactName =           "pitchRate";
const char* Vehicle::_yawRateFactName =             "yawRate";
const char* Vehicle::_airSpeedFactName =            "airSpeed";
const char* Vehicle::_groundSpeedFactName =         "groundSpeed";
const char* Vehicle::_climbRateFactName =           "climbRate";
const char* Vehicle::_altitudeRelativeFactName =    "altitudeRelative";
const char* Vehicle::_altitudeAMSLFactName =        "altitudeAMSL";
const char* Vehicle::_flightDistanceFactName =      "flightDistance";
const char* Vehicle::_flightTimeFactName =          "flightTime";
const char* Vehicle::_distanceToHomeFactName =      "distanceToHome";
const char* Vehicle::_hobbsFactName =               "hobbs";

const char* Vehicle::_gpsFactGroupName =            "gps";
const char* Vehicle::_battery1FactGroupName =       "battery";
const char* Vehicle::_battery2FactGroupName =       "battery2";
const char* Vehicle::_windFactGroupName =           "wind";
const char* Vehicle::_vibrationFactGroupName =      "vibration";
const char* Vehicle::_temperatureFactGroupName =    "temperature";
const char* Vehicle::_clockFactGroupName =          "clock";
const char* Vehicle::_distanceSensorFactGroupName = "distanceSensor";

Vehicle::Vehicle(LinkInterface*             link,
                 int                        vehicleId,
                 int                        defaultComponentId,
                 MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 JoystickManager*           joystickManager)
    : FactGroup(_vehicleUIUpdateRateMSecs, ":/json/Vehicle/VehicleFact.json")
    , _id(vehicleId)
    , _defaultComponentId(defaultComponentId)
    , _active(false)
    , _offlineEditingVehicle(false)
    , _firmwareType(firmwareType)
    , _vehicleType(vehicleType)
    , _firmwarePlugin(NULL)
    , _firmwarePluginInstanceData(NULL)
    , _autopilotPlugin(NULL)
    , _mavlink(NULL)
    , _soloFirmware(false)
    , _toolbox(qgcApp()->toolbox())
    , _settingsManager(_toolbox->settingsManager())
    , _joystickMode(JoystickModeRC)
    , _joystickEnabled(false)
    , _uas(NULL)
    , _mav(NULL)
    , _currentMessageCount(0)
    , _messageCount(0)
    , _currentErrorCount(0)
    , _currentWarningCount(0)
    , _currentNormalCount(0)
    , _currentMessageType(MessageNone)
    , _updateCount(0)
    , _rcRSSI(255)
    , _rcRSSIstore(255.)
    , _autoDisconnect(false)
    , _flying(false)
    , _landing(false)
    , _vtolInFwdFlight(false)
    , _onboardControlSensorsPresent(0)
    , _onboardControlSensorsEnabled(0)
    , _onboardControlSensorsHealth(0)
    , _onboardControlSensorsUnhealthy(0)
    , _gpsRawIntMessageAvailable(false)
    , _globalPositionIntMessageAvailable(false)
    , _defaultCruiseSpeed(_settingsManager->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed(_settingsManager->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _telemetryRRSSI(0)
    , _telemetryLRSSI(0)
    , _telemetryRXErrors(0)
    , _telemetryFixed(0)
    , _telemetryTXBuffer(0)
    , _telemetryLNoise(0)
    , _telemetryRNoise(0)
    , _maxProtoVersion(0)
    , _vehicleCapabilitiesKnown(false)
    , _capabilityBits(0)
    , _highLatencyLink(false)
    , _receivingAttitudeQuaternion(false)
    , _cameras(NULL)
    , _connectionLost(false)
    , _connectionLostEnabled(true)
    , _initialPlanRequestComplete(false)
    , _missionManager(NULL)
    , _missionManagerInitialRequestSent(false)
    , _geoFenceManager(NULL)
    , _geoFenceManagerInitialRequestSent(false)
    , _rallyPointManager(NULL)
    , _rallyPointManagerInitialRequestSent(false)
    , _parameterManager(NULL)
    , _armed(false)
    , _base_mode(0)
    , _custom_mode(0)
    , _nextSendMessageMultipleIndex(0)
    , _firmwarePluginManager(firmwarePluginManager)
    , _joystickManager(joystickManager)
    , _flowImageIndex(0)
    , _allLinksInactiveSent(false)
    , _messagesReceived(0)
    , _messagesSent(0)
    , _messagesLost(0)
    , _messageSeq(0)
    , _compID(0)
    , _heardFrom(false)
    , _firmwareMajorVersion(versionNotSetValue)
    , _firmwareMinorVersion(versionNotSetValue)
    , _firmwarePatchVersion(versionNotSetValue)
    , _firmwareCustomMajorVersion(versionNotSetValue)
    , _firmwareCustomMinorVersion(versionNotSetValue)
    , _firmwareCustomPatchVersion(versionNotSetValue)
    , _firmwareVersionType(FIRMWARE_VERSION_TYPE_OFFICIAL)
    , _gitHash(versionNotSetValue)
    , _uid(0)
    , _lastAnnouncedLowBatteryPercent(100)
    , _priorityLinkCommanded(false)
    , _rollFact             (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact            (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact          (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _rollRateFact         (0, _rollRateFactName,          FactMetaData::valueTypeDouble)
    , _pitchRateFact        (0, _pitchRateFactName,         FactMetaData::valueTypeDouble)
    , _yawRateFact          (0, _yawRateFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact      (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact         (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _climbRateFact        (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact     (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _flightDistanceFact   (0, _flightDistanceFactName,    FactMetaData::valueTypeDouble)
    , _flightTimeFact       (0, _flightTimeFactName,        FactMetaData::valueTypeElapsedTimeInSeconds)
    , _distanceToHomeFact   (0, _distanceToHomeFactName,    FactMetaData::valueTypeDouble)
    , _hobbsFact            (0, _hobbsFactName,             FactMetaData::valueTypeString)
    , _gpsFactGroup(this)
    , _battery1FactGroup(this)
    , _battery2FactGroup(this)
    , _windFactGroup(this)
    , _vibrationFactGroup(this)
    , _temperatureFactGroup(this)
    , _clockFactGroup(this)
    , _distanceSensorFactGroup(this)
{
    connect(_joystickManager, &JoystickManager::activeJoystickChanged, this, &Vehicle::_loadSettings);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleAvailableChanged, this, &Vehicle::_loadSettings);

    _mavlink = _toolbox->mavlinkProtocol();

    connect(_mavlink, &MAVLinkProtocol::messageReceived,     this, &Vehicle::_mavlinkMessageReceived);

    _addLink(link);

    connect(this, &Vehicle::_sendMessageOnLinkOnThread, this, &Vehicle::_sendMessageOnLink, Qt::QueuedConnection);
    connect(this, &Vehicle::flightModeChanged,          this, &Vehicle::_handleFlightModeChanged);
    connect(this, &Vehicle::armedChanged,               this, &Vehicle::_announceArmedChanged);

    connect(_toolbox->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &Vehicle::_vehicleParamLoaded);

    _uas = new UAS(_mavlink, this, _firmwarePluginManager);

    connect(_uas, &UAS::imageReady,                     this, &Vehicle::_imageReady);
    connect(this, &Vehicle::remoteControlRSSIChanged,   this, &Vehicle::_remoteControlRSSIChanged);

    _commonInit();
    _autopilotPlugin = _firmwarePlugin->autopilotPlugin(this);

    // connect this vehicle to the follow me handle manager
    connect(this, &Vehicle::flightModeChanged,_toolbox->followMe(), &FollowMe::followMeHandleManager);

    // PreArm Error self-destruct timer
    connect(&_prearmErrorTimer, &QTimer::timeout, this, &Vehicle::_prearmErrorTimeout);
    _prearmErrorTimer.setInterval(_prearmErrorTimeoutMSecs);
    _prearmErrorTimer.setSingleShot(true);

    // Send MAV_CMD ack timer
    _mavCommandAckTimer.setSingleShot(true);
    _mavCommandAckTimer.setInterval(_highLatencyLink ? _mavCommandAckTimeoutMSecsHighLatency : _mavCommandAckTimeoutMSecs);
    connect(&_mavCommandAckTimer, &QTimer::timeout, this, &Vehicle::_sendMavCommandAgain);

    _mav = uas();

    // Listen for system messages
    connect(_toolbox->uasMessageHandler(), &UASMessageHandler::textMessageCountChanged,  this, &Vehicle::_handleTextMessage);
    connect(_toolbox->uasMessageHandler(), &UASMessageHandler::textMessageReceived,      this, &Vehicle::_handletextMessageReceived);

    if (_highLatencyLink || link->isPX4Flow()) {
        // These links don't request information
        _setMaxProtoVersion(100);
        _setCapabilities(0);
        _initialPlanRequestComplete = true;
        _missionManagerInitialRequestSent = true;
        _geoFenceManagerInitialRequestSent = true;
        _rallyPointManagerInitialRequestSent = true;
    } else {
        // Ask the vehicle for protocol version info.
        sendMavCommand(MAV_COMP_ID_ALL,                     // Don't know default component id yet.
                       MAV_CMD_REQUEST_PROTOCOL_VERSION,
                       false,                               // No error shown if fails
                       1);                                  // Request protocol version

        // Ask the vehicle for firmware version info.
        sendMavCommand(MAV_COMP_ID_ALL,                         // Don't know default component id yet.
                       MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES,
                       false,                                   // No error shown if fails
                       1);                                      // Request firmware version
    }

    _firmwarePlugin->initializeVehicle(this);

    _sendMultipleTimer.start(_sendMessageMultipleIntraMessageDelay);
    connect(&_sendMultipleTimer, &QTimer::timeout, this, &Vehicle::_sendMessageMultipleNext);

    _mapTrajectoryTimer.setInterval(_mapTrajectoryMsecsBetweenPoints);
    connect(&_mapTrajectoryTimer, &QTimer::timeout, this, &Vehicle::_addNewMapTrajectoryPoint);

    // Create camera manager instance
    _cameras = _firmwarePlugin->createCameraManager(this);
    emit dynamicCamerasChanged();
}

// Disconnected Vehicle for offline editing
Vehicle::Vehicle(MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 QObject*                   parent)
    : FactGroup(_vehicleUIUpdateRateMSecs, ":/json/Vehicle/VehicleFact.json", parent)
    , _id(0)
    , _defaultComponentId(MAV_COMP_ID_ALL)
    , _active(false)
    , _offlineEditingVehicle(true)
    , _firmwareType(firmwareType)
    , _vehicleType(vehicleType)
    , _firmwarePlugin(NULL)
    , _firmwarePluginInstanceData(NULL)
    , _autopilotPlugin(NULL)
    , _mavlink(NULL)
    , _soloFirmware(false)
    , _toolbox(qgcApp()->toolbox())
    , _settingsManager(_toolbox->settingsManager())
    , _joystickMode(JoystickModeRC)
    , _joystickEnabled(false)
    , _uas(NULL)
    , _mav(NULL)
    , _currentMessageCount(0)
    , _messageCount(0)
    , _currentErrorCount(0)
    , _currentWarningCount(0)
    , _currentNormalCount(0)
    , _currentMessageType(MessageNone)
    , _updateCount(0)
    , _rcRSSI(255)
    , _rcRSSIstore(255.)
    , _autoDisconnect(false)
    , _flying(false)
    , _landing(false)
    , _vtolInFwdFlight(false)
    , _onboardControlSensorsPresent(0)
    , _onboardControlSensorsEnabled(0)
    , _onboardControlSensorsHealth(0)
    , _onboardControlSensorsUnhealthy(0)
    , _gpsRawIntMessageAvailable(false)
    , _globalPositionIntMessageAvailable(false)
    , _defaultCruiseSpeed(_settingsManager->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed(_settingsManager->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _vehicleCapabilitiesKnown(true)
    , _capabilityBits(_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA ? 0 : MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY)
    , _highLatencyLink(false)
    , _receivingAttitudeQuaternion(false)
    , _cameras(NULL)
    , _connectionLost(false)
    , _connectionLostEnabled(true)
    , _initialPlanRequestComplete(false)
    , _missionManager(NULL)
    , _missionManagerInitialRequestSent(false)
    , _geoFenceManager(NULL)
    , _geoFenceManagerInitialRequestSent(false)
    , _rallyPointManager(NULL)
    , _rallyPointManagerInitialRequestSent(false)
    , _parameterManager(NULL)
    , _armed(false)
    , _base_mode(0)
    , _custom_mode(0)
    , _nextSendMessageMultipleIndex(0)
    , _firmwarePluginManager(firmwarePluginManager)
    , _joystickManager(NULL)
    , _flowImageIndex(0)
    , _allLinksInactiveSent(false)
    , _messagesReceived(0)
    , _messagesSent(0)
    , _messagesLost(0)
    , _messageSeq(0)
    , _compID(0)
    , _heardFrom(false)
    , _firmwareMajorVersion(versionNotSetValue)
    , _firmwareMinorVersion(versionNotSetValue)
    , _firmwarePatchVersion(versionNotSetValue)
    , _firmwareCustomMajorVersion(versionNotSetValue)
    , _firmwareCustomMinorVersion(versionNotSetValue)
    , _firmwareCustomPatchVersion(versionNotSetValue)
    , _firmwareVersionType(FIRMWARE_VERSION_TYPE_OFFICIAL)
    , _gitHash(versionNotSetValue)
    , _uid(0)
    , _lastAnnouncedLowBatteryPercent(100)
    , _rollFact             (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact            (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact          (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _rollRateFact         (0, _rollRateFactName,          FactMetaData::valueTypeDouble)
    , _pitchRateFact        (0, _pitchRateFactName,         FactMetaData::valueTypeDouble)
    , _yawRateFact          (0, _yawRateFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact      (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact         (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _climbRateFact        (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact     (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _flightDistanceFact   (0, _flightDistanceFactName,    FactMetaData::valueTypeDouble)
    , _flightTimeFact       (0, _flightTimeFactName,        FactMetaData::valueTypeElapsedTimeInSeconds)
    , _distanceToHomeFact   (0, _distanceToHomeFactName,    FactMetaData::valueTypeDouble)
    , _hobbsFact            (0, _hobbsFactName,             FactMetaData::valueTypeString)
    , _gpsFactGroup(this)
    , _battery1FactGroup(this)
    , _battery2FactGroup(this)
    , _windFactGroup(this)
    , _vibrationFactGroup(this)
    , _clockFactGroup(this)
    , _distanceSensorFactGroup(this)
{
    _commonInit();
    _firmwarePlugin->initializeVehicle(this);
}

void Vehicle::_commonInit(void)
{
    _firmwarePlugin = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);

    connect(_firmwarePlugin, &FirmwarePlugin::toolbarIndicatorsChanged, this, &Vehicle::toolBarIndicatorsChanged);

    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateDistanceToHome);
    connect(this, &Vehicle::homePositionChanged,    this, &Vehicle::_updateDistanceToHome);
    connect(this, &Vehicle::hobbsMeterChanged,      this, &Vehicle::_updateHobbsMeter);

    _missionManager = new MissionManager(this);
    connect(_missionManager, &MissionManager::error,                    this, &Vehicle::_missionManagerError);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_missionLoadComplete);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_clearCameraTriggerPoints);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_clearTrajectoryPoints);
    connect(_missionManager, &MissionManager::sendComplete,             this, &Vehicle::_clearCameraTriggerPoints);
    connect(_missionManager, &MissionManager::sendComplete,             this, &Vehicle::_clearTrajectoryPoints);

    _parameterManager = new ParameterManager(this);
    connect(_parameterManager, &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);

    // GeoFenceManager needs to access ParameterManager so make sure to create after
    _geoFenceManager = new GeoFenceManager(this);
    connect(_geoFenceManager, &GeoFenceManager::error,          this, &Vehicle::_geoFenceManagerError);
    connect(_geoFenceManager, &GeoFenceManager::loadComplete,   this, &Vehicle::_geoFenceLoadComplete);

    _rallyPointManager = new RallyPointManager(this);
    connect(_rallyPointManager, &RallyPointManager::error,          this, &Vehicle::_rallyPointManagerError);
    connect(_rallyPointManager, &RallyPointManager::loadComplete,   this, &Vehicle::_rallyPointLoadComplete);

    // Offline editing vehicle tracks settings changes for offline editing settings
    connect(_settingsManager->appSettings()->offlineEditingFirmwareType(),  &Fact::rawValueChanged, this, &Vehicle::_offlineFirmwareTypeSettingChanged);
    connect(_settingsManager->appSettings()->offlineEditingVehicleType(),   &Fact::rawValueChanged, this, &Vehicle::_offlineVehicleTypeSettingChanged);
    connect(_settingsManager->appSettings()->offlineEditingCruiseSpeed(),   &Fact::rawValueChanged, this, &Vehicle::_offlineCruiseSpeedSettingChanged);
    connect(_settingsManager->appSettings()->offlineEditingHoverSpeed(),    &Fact::rawValueChanged, this, &Vehicle::_offlineHoverSpeedSettingChanged);

    // Flight modes can differ based on advanced mode
    connect(_toolbox->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &Vehicle::flightModesChanged);

    // Build FactGroup object model

    _addFact(&_rollFact,                _rollFactName);
    _addFact(&_pitchFact,               _pitchFactName);
    _addFact(&_headingFact,             _headingFactName);
    _addFact(&_rollRateFact,            _rollRateFactName);
    _addFact(&_pitchRateFact,           _pitchRateFactName);
    _addFact(&_yawRateFact,             _yawRateFactName);
    _addFact(&_groundSpeedFact,         _groundSpeedFactName);
    _addFact(&_airSpeedFact,            _airSpeedFactName);
    _addFact(&_climbRateFact,           _climbRateFactName);
    _addFact(&_altitudeRelativeFact,    _altitudeRelativeFactName);
    _addFact(&_altitudeAMSLFact,        _altitudeAMSLFactName);
    _addFact(&_flightDistanceFact,      _flightDistanceFactName);
    _addFact(&_flightTimeFact,          _flightTimeFactName);
    _addFact(&_distanceToHomeFact,      _distanceToHomeFactName);

    _hobbsFact.setRawValue(QVariant(QString("0000:00:00")));
    _addFact(&_hobbsFact,               _hobbsFactName);

    _addFactGroup(&_gpsFactGroup,               _gpsFactGroupName);
    _addFactGroup(&_battery1FactGroup,          _battery1FactGroupName);
    _addFactGroup(&_battery2FactGroup,          _battery2FactGroupName);
    _addFactGroup(&_windFactGroup,              _windFactGroupName);
    _addFactGroup(&_vibrationFactGroup,         _vibrationFactGroupName);
    _addFactGroup(&_temperatureFactGroup,       _temperatureFactGroupName);
    _addFactGroup(&_clockFactGroup,             _clockFactGroupName);
    _addFactGroup(&_distanceSensorFactGroup,    _distanceSensorFactGroupName);

    // Add firmware-specific fact groups, if provided
    QMap<QString, FactGroup*>* fwFactGroups = _firmwarePlugin->factGroups();
    if (fwFactGroups) {
        QMapIterator<QString, FactGroup*> i(*fwFactGroups);
        while(i.hasNext()) {
            i.next();
            _addFactGroup(i.value(), i.key());
        }
    }

    _flightDistanceFact.setRawValue(0);
    _flightTimeFact.setRawValue(0);
}

Vehicle::~Vehicle()
{
    qCDebug(VehicleLog) << "~Vehicle" << this;

    delete _missionManager;
    _missionManager = NULL;

    delete _autopilotPlugin;
    _autopilotPlugin = NULL;

    delete _mav;
    _mav = NULL;

}

void Vehicle::prepareDelete()
{
    if(_cameras) {
        delete _cameras;
        _cameras = NULL;
        emit dynamicCamerasChanged();
        qApp->processEvents();
    }
}

void Vehicle::_offlineFirmwareTypeSettingChanged(QVariant value)
{
    _firmwareType = static_cast<MAV_AUTOPILOT>(value.toInt());
    _firmwarePlugin = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);
    emit firmwareTypeChanged();
    if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        _capabilityBits = 0;
    } else {
        _capabilityBits = MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY;
    }
    emit capabilityBitsChanged(_capabilityBits);
}

void Vehicle::_offlineVehicleTypeSettingChanged(QVariant value)
{
    _vehicleType = static_cast<MAV_TYPE>(value.toInt());
    emit vehicleTypeChanged();
}

void Vehicle::_offlineCruiseSpeedSettingChanged(QVariant value)
{
    _defaultCruiseSpeed = value.toDouble();
    emit defaultCruiseSpeedChanged(_defaultCruiseSpeed);
}

void Vehicle::_offlineHoverSpeedSettingChanged(QVariant value)
{
    _defaultHoverSpeed = value.toDouble();
    emit defaultHoverSpeedChanged(_defaultHoverSpeed);
}

QString Vehicle::firmwareTypeString(void) const
{
    if (px4Firmware()) {
        return QStringLiteral("PX4 Pro");
    } else if (apmFirmware()) {
        return QStringLiteral("ArduPilot");
    } else {
        return tr("MAVLink Generic");
    }
}

QString Vehicle::vehicleTypeString(void) const
{
    if (fixedWing()) {
        return tr("Fixed Wing");
    } else if (multiRotor()) {
        return tr("Multi-Rotor");
    } else if (vtol()) {
        return tr("VTOL");
    } else if (rover()) {
        return tr("Rover");
    } else if (sub()) {
        return tr("Sub");
    } else {
        return tr("Unknown");
    }
}

void Vehicle::resetCounters()
{
    _messagesReceived   = 0;
    _messagesSent       = 0;
    _messagesLost       = 0;
    _messageSeq         = 0;
    _heardFrom          = false;
}

void Vehicle::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    // if the minimum supported version of MAVLink is already 2.0
    // set our max proto version to it.
    unsigned mavlinkVersion = _mavlink->getCurrentVersion();
    if (_maxProtoVersion != mavlinkVersion && mavlinkVersion >= 200) {
        _maxProtoVersion = _mavlink->getCurrentVersion();
        qCDebug(VehicleLog) << "Vehicle::_mavlinkMessageReceived setting _maxProtoVersion" << _maxProtoVersion;
    }

    if (message.sysid != _id && message.sysid != 0) {
        // We allow RADIO_STATUS messages which come from a link the vehicle is using to pass through and be handled
        if (!(message.msgid == MAVLINK_MSG_ID_RADIO_STATUS && _containsLink(link))) {
            return;
        }
    }

    if (!_containsLink(link)) {
        _addLink(link);
    }

    //-- Check link status
    _messagesReceived++;
    emit messagesReceivedChanged();
    if(!_heardFrom) {
        if(message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            _heardFrom = true;
            _compID = message.compid;
            _messageSeq = message.seq + 1;
        }
    } else {
        if(_compID == message.compid) {
            uint16_t seq_received = (uint16_t)message.seq;
            uint16_t packet_lost_count = 0;
            //-- Account for overflow during packet loss
            if(seq_received < _messageSeq) {
                packet_lost_count = (seq_received + 255) - _messageSeq;
            } else {
                packet_lost_count = seq_received - _messageSeq;
            }
            _messageSeq = message.seq + 1;
            _messagesLost += packet_lost_count;
            if(packet_lost_count)
                emit messagesLostChanged();
        }
    }

    // Give the plugin a change to adjust the message contents
    if (!_firmwarePlugin->adjustIncomingMavlinkMessage(this, &message)) {
        return;
    }

    // Give the Core Plugin access to all mavlink traffic
    if (!_toolbox->corePlugin()->mavlinkMessage(this, link, message)) {
        return;
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HOME_POSITION:
        _handleHomePosition(message);
        break;
    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartbeat(message);
        break;
    case MAVLINK_MSG_ID_RADIO_STATUS:
        _handleRadioStatus(message);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS:
        _handleRCChannels(message);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
        _handleRCChannelsRaw(message);
        break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
        _handleBatteryStatus(message);
        break;
    case MAVLINK_MSG_ID_SYS_STATUS:
        _handleSysStatus(message);
        break;
    case MAVLINK_MSG_ID_RAW_IMU:
        emit mavlinkRawImu(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU:
        emit mavlinkScaledImu1(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU2:
        emit mavlinkScaledImu2(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU3:
        emit mavlinkScaledImu3(message);
        break;
    case MAVLINK_MSG_ID_VIBRATION:
        _handleVibration(message);
        break;
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE:
        _handleExtendedSysState(message);
        break;
    case MAVLINK_MSG_ID_COMMAND_ACK:
        _handleCommandAck(message);
        break;
    case MAVLINK_MSG_ID_COMMAND_LONG:
        _handleCommandLong(message);
        break;
    case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
        _handleAutopilotVersion(link, message);
        break;
    case MAVLINK_MSG_ID_PROTOCOL_VERSION:
        _handleProtocolVersion(link, message);
        break;
    case MAVLINK_MSG_ID_WIND_COV:
        _handleWindCov(message);
        break;
    case MAVLINK_MSG_ID_HIL_ACTUATOR_CONTROLS:
        _handleHilActuatorControls(message);
        break;
    case MAVLINK_MSG_ID_LOGGING_DATA:
        _handleMavlinkLoggingData(message);
        break;
    case MAVLINK_MSG_ID_LOGGING_DATA_ACKED:
        _handleMavlinkLoggingDataAcked(message);
        break;
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        _handleGpsRawInt(message);
        break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        _handleGlobalPositionInt(message);
        break;
    case MAVLINK_MSG_ID_ALTITUDE:
        _handleAltitude(message);
        break;
    case MAVLINK_MSG_ID_VFR_HUD:
        _handleVfrHud(message);
        break;
    case MAVLINK_MSG_ID_SCALED_PRESSURE:
        _handleScaledPressure(message);
        break;
    case MAVLINK_MSG_ID_SCALED_PRESSURE2:
        _handleScaledPressure2(message);
        break;
    case MAVLINK_MSG_ID_SCALED_PRESSURE3:
        _handleScaledPressure3(message);
        break;
    case MAVLINK_MSG_ID_CAMERA_IMAGE_CAPTURED:
        _handleCameraImageCaptured(message);
        break;
    case MAVLINK_MSG_ID_ADSB_VEHICLE:
        _handleADSBVehicle(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(message);
        break;
    case MAVLINK_MSG_ID_ATTITUDE:
        _handleAttitude(message);
        break;
    case MAVLINK_MSG_ID_ATTITUDE_QUATERNION:
        _handleAttitudeQuaternion(message);
        break;
    case MAVLINK_MSG_ID_ATTITUDE_TARGET:
        _handleAttitudeTarget(message);
        break;
    case MAVLINK_MSG_ID_DISTANCE_SENSOR:
        _handleDistanceSensor(message);
        break;
    case MAVLINK_MSG_ID_PING:
        _handlePing(link, message);
        break;

    case MAVLINK_MSG_ID_SERIAL_CONTROL:
    {
        mavlink_serial_control_t ser;
        mavlink_msg_serial_control_decode(&message, &ser);
        emit mavlinkSerialControl(ser.device, ser.flags, ser.timeout, ser.baudrate, QByteArray(reinterpret_cast<const char*>(ser.data), ser.count));
    }
        break;

        // Following are ArduPilot dialect messages
#if !defined(NO_ARDUPILOT_DIALECT)
    case MAVLINK_MSG_ID_CAMERA_FEEDBACK:
        _handleCameraFeedback(message);
        break;
    case MAVLINK_MSG_ID_WIND:
        _handleWind(message);
        break;
#endif
    }

    // This must be emitted after the vehicle processes the message. This way the vehicle state is up to date when anyone else
    // does processing.
    emit mavlinkMessageReceived(message);

    _uas->receiveMessage(message);
}


#if !defined(NO_ARDUPILOT_DIALECT)
void Vehicle::_handleCameraFeedback(const mavlink_message_t& message)
{
    mavlink_camera_feedback_t feedback;

    mavlink_msg_camera_feedback_decode(&message, &feedback);

    QGeoCoordinate imageCoordinate((double)feedback.lat / qPow(10.0, 7.0), (double)feedback.lng / qPow(10.0, 7.0), feedback.alt_msl);
    qCDebug(VehicleLog) << "_handleCameraFeedback coord:index" << imageCoordinate << feedback.img_idx;
    _cameraTriggerPoints.append(new QGCQGeoCoordinate(imageCoordinate, this));
}
#endif

void Vehicle::_handleCameraImageCaptured(const mavlink_message_t& message)
{
    mavlink_camera_image_captured_t feedback;

    mavlink_msg_camera_image_captured_decode(&message, &feedback);

    QGeoCoordinate imageCoordinate((double)feedback.lat / qPow(10.0, 7.0), (double)feedback.lon / qPow(10.0, 7.0), feedback.alt);
    qCDebug(VehicleLog) << "_handleCameraFeedback coord:index" << imageCoordinate << feedback.image_index << feedback.capture_result;
    if (feedback.capture_result == 1) {
        _cameraTriggerPoints.append(new QGCQGeoCoordinate(imageCoordinate, this));
    }
}

void Vehicle::_handleVfrHud(mavlink_message_t& message)
{
    mavlink_vfr_hud_t vfrHud;
    mavlink_msg_vfr_hud_decode(&message, &vfrHud);

    _airSpeedFact.setRawValue(qIsNaN(vfrHud.airspeed) ? 0 : vfrHud.airspeed);
    _groundSpeedFact.setRawValue(qIsNaN(vfrHud.groundspeed) ? 0 : vfrHud.groundspeed);
    _climbRateFact.setRawValue(qIsNaN(vfrHud.climb) ? 0 : vfrHud.climb);
}

void Vehicle::_handleDistanceSensor(mavlink_message_t& message)
{
    mavlink_distance_sensor_t distanceSensor;

    mavlink_msg_distance_sensor_decode(&message, &distanceSensor);\

    if (!_distanceSensorFactGroup.idSet()) {
        _distanceSensorFactGroup.setIdSet(true);
        _distanceSensorFactGroup.setId(distanceSensor.id);
    }

    if (_distanceSensorFactGroup.id() != distanceSensor.id) {
        // We can only handle a single sensor reporting
        return;
    }

    struct orientation2Fact_s {
        MAV_SENSOR_ORIENTATION  orientation;
        Fact*                   fact;
    };

    orientation2Fact_s rgOrientation2Fact[] =
    {
        { MAV_SENSOR_ROTATION_NONE,         _distanceSensorFactGroup.rotationNone() },
        { MAV_SENSOR_ROTATION_YAW_45,       _distanceSensorFactGroup.rotationYaw45() },
        { MAV_SENSOR_ROTATION_YAW_90,       _distanceSensorFactGroup.rotationYaw90() },
        { MAV_SENSOR_ROTATION_YAW_135,      _distanceSensorFactGroup.rotationYaw135() },
        { MAV_SENSOR_ROTATION_YAW_180,      _distanceSensorFactGroup.rotationYaw180() },
        { MAV_SENSOR_ROTATION_YAW_225,      _distanceSensorFactGroup.rotationYaw225() },
        { MAV_SENSOR_ROTATION_YAW_270,      _distanceSensorFactGroup.rotationYaw270() },
        { MAV_SENSOR_ROTATION_YAW_315,      _distanceSensorFactGroup.rotationYaw315() },
        { MAV_SENSOR_ROTATION_PITCH_90,     _distanceSensorFactGroup.rotationPitch90() },
        { MAV_SENSOR_ROTATION_PITCH_270,    _distanceSensorFactGroup.rotationPitch270() },
    };

    for (size_t i=0; i<sizeof(rgOrientation2Fact)/sizeof(rgOrientation2Fact[0]); i++) {
        const orientation2Fact_s& orientation2Fact = rgOrientation2Fact[i];
        if (orientation2Fact.orientation == distanceSensor.orientation) {
            orientation2Fact.fact->setRawValue(distanceSensor.current_distance / 100.0); // cm to meters
        }
    }
}

void Vehicle::_handleAttitudeTarget(mavlink_message_t& message)
{
    mavlink_attitude_target_t attitudeTarget;

    mavlink_msg_attitude_target_decode(&message, &attitudeTarget);

    float roll, pitch, yaw;
    mavlink_quaternion_to_euler(attitudeTarget.q, &roll, &pitch, &yaw);

    _setpointFactGroup.roll()->setRawValue(qRadiansToDegrees(roll));
    _setpointFactGroup.pitch()->setRawValue(qRadiansToDegrees(pitch));
    _setpointFactGroup.yaw()->setRawValue(qRadiansToDegrees(yaw));

    _setpointFactGroup.rollRate()->setRawValue(qRadiansToDegrees(attitudeTarget.body_roll_rate));
    _setpointFactGroup.pitchRate()->setRawValue(qRadiansToDegrees(attitudeTarget.body_pitch_rate));
    _setpointFactGroup.yawRate()->setRawValue(qRadiansToDegrees(attitudeTarget.body_yaw_rate));
}

void Vehicle::_handleAttitudeWorker(double rollRadians, double pitchRadians, double yawRadians)
{
    double roll, pitch, yaw;

    roll = QGC::limitAngleToPMPIf(rollRadians);
    pitch = QGC::limitAngleToPMPIf(pitchRadians);
    yaw = QGC::limitAngleToPMPIf(yawRadians);

    roll = qRadiansToDegrees(roll);
    pitch = qRadiansToDegrees(pitch);
    yaw = qRadiansToDegrees(yaw);

    if (yaw < 0.0) {
        yaw += 360.0;
    }
    // truncate to integer so widget never displays 360
    yaw = trunc(yaw);

    _rollFact.setRawValue(roll);
    _pitchFact.setRawValue(pitch);
    _headingFact.setRawValue(yaw);
}

void Vehicle::_handleAttitude(mavlink_message_t& message)
{
    if (_receivingAttitudeQuaternion) {
        return;
    }

    mavlink_attitude_t attitude;
    mavlink_msg_attitude_decode(&message, &attitude);

    _handleAttitudeWorker(attitude.roll, attitude.pitch, attitude.yaw);
}

void Vehicle::_handleAttitudeQuaternion(mavlink_message_t& message)
{
    _receivingAttitudeQuaternion = true;

    mavlink_attitude_quaternion_t attitudeQuaternion;
    mavlink_msg_attitude_quaternion_decode(&message, &attitudeQuaternion);

    float roll, pitch, yaw;
    float q[] = { attitudeQuaternion.q1, attitudeQuaternion.q2, attitudeQuaternion.q3, attitudeQuaternion.q4 };
    mavlink_quaternion_to_euler(q, &roll, &pitch, &yaw);

    _handleAttitudeWorker(roll, pitch, yaw);

    rollRate()->setRawValue(qRadiansToDegrees(attitudeQuaternion.rollspeed));
    pitchRate()->setRawValue(qRadiansToDegrees(attitudeQuaternion.pitchspeed));
    yawRate()->setRawValue(qRadiansToDegrees(attitudeQuaternion.yawspeed));
}

void Vehicle::_handleGpsRawInt(mavlink_message_t& message)
{
    mavlink_gps_raw_int_t gpsRawInt;
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);

    _gpsRawIntMessageAvailable = true;

    if (gpsRawInt.fix_type >= GPS_FIX_TYPE_3D_FIX) {
        if (!_globalPositionIntMessageAvailable) {
            QGeoCoordinate newPosition(gpsRawInt.lat  / (double)1E7, gpsRawInt.lon / (double)1E7, gpsRawInt.alt  / 1000.0);
            if (newPosition != _coordinate) {
                _coordinate = newPosition;
                emit coordinateChanged(_coordinate);
            }
            _altitudeAMSLFact.setRawValue(gpsRawInt.alt / 1000.0);
        }
    }

    _gpsFactGroup.lat()->setRawValue(gpsRawInt.lat * 1e-7);
    _gpsFactGroup.lon()->setRawValue(gpsRawInt.lon * 1e-7);
    _gpsFactGroup.count()->setRawValue(gpsRawInt.satellites_visible == 255 ? 0 : gpsRawInt.satellites_visible);
    _gpsFactGroup.hdop()->setRawValue(gpsRawInt.eph == UINT16_MAX ? std::numeric_limits<double>::quiet_NaN() : gpsRawInt.eph / 100.0);
    _gpsFactGroup.vdop()->setRawValue(gpsRawInt.epv == UINT16_MAX ? std::numeric_limits<double>::quiet_NaN() : gpsRawInt.epv / 100.0);
    _gpsFactGroup.courseOverGround()->setRawValue(gpsRawInt.cog == UINT16_MAX ? std::numeric_limits<double>::quiet_NaN() : gpsRawInt.cog / 100.0);
    _gpsFactGroup.lock()->setRawValue(gpsRawInt.fix_type);
}

void Vehicle::_handleGlobalPositionInt(mavlink_message_t& message)
{
    mavlink_global_position_int_t globalPositionInt;
    mavlink_msg_global_position_int_decode(&message, &globalPositionInt);

    _altitudeRelativeFact.setRawValue(globalPositionInt.relative_alt / 1000.0);
    _altitudeAMSLFact.setRawValue(globalPositionInt.alt / 1000.0);

    // ArduPilot sends bogus GLOBAL_POSITION_INT messages with lat/lat 0/0 even when it has no gps signal
    // Apparently, this is in order to transport relative altitude information.
    if (globalPositionInt.lat == 0 && globalPositionInt.lon == 0) {
        return;
    }

    _globalPositionIntMessageAvailable = true;
    QGeoCoordinate newPosition(globalPositionInt.lat  / (double)1E7, globalPositionInt.lon / (double)1E7, globalPositionInt.alt  / 1000.0);
    if (newPosition != _coordinate) {
        _coordinate = newPosition;
        emit coordinateChanged(_coordinate);
    }
}

void Vehicle::_handleHighLatency2(mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    QString previousFlightMode;
    if (_base_mode != 0 || _custom_mode != 0){
        // Vehicle is initialized with _base_mode=0 and _custom_mode=0. Don't pass this to flightMode() since it will complain about
        // bad modes while unit testing.
        previousFlightMode = flightMode();
    }
    _base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    _custom_mode = _firmwarePlugin->highLatencyCustomModeTo32Bits(highLatency2.custom_mode);
    if (previousFlightMode != flightMode()) {
        emit flightModeChanged(flightMode());
    }

    // Assume armed since we don't know
    if (_armed != true) {
        _armed = true;
        emit armedChanged(_armed);
    }

    _coordinate.setLatitude(highLatency2.latitude  / (double)1E7);
    _coordinate.setLongitude(highLatency2.longitude / (double)1E7);
    _coordinate.setAltitude(highLatency2.altitude);
    emit coordinateChanged(_coordinate);

    _airSpeedFact.setRawValue((double)highLatency2.airspeed / 5.0);
    _groundSpeedFact.setRawValue((double)highLatency2.groundspeed / 5.0);
    _climbRateFact.setRawValue((double)highLatency2.climb_rate / 10.0);
    _headingFact.setRawValue((double)highLatency2.heading * 2.0);
    _altitudeRelativeFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _altitudeAMSLFact.setRawValue(highLatency2.altitude);

    _windFactGroup.direction()->setRawValue((double)highLatency2.wind_heading * 2.0);
    _windFactGroup.speed()->setRawValue((double)highLatency2.windspeed / 5.0);

    _battery1FactGroup.percentRemaining()->setRawValue(highLatency2.battery);

    _temperatureFactGroup.temperature1()->setRawValue(highLatency2.temperature_air);

    _gpsFactGroup.lat()->setRawValue(highLatency2.latitude * 1e-7);
    _gpsFactGroup.lon()->setRawValue(highLatency2.longitude * 1e-7);
    _gpsFactGroup.count()->setRawValue(0);
    _gpsFactGroup.hdop()->setRawValue(highLatency2.eph == UINT8_MAX ? std::numeric_limits<double>::quiet_NaN() : highLatency2.eph / 10.0);
    _gpsFactGroup.vdop()->setRawValue(highLatency2.epv == UINT8_MAX ? std::numeric_limits<double>::quiet_NaN() : highLatency2.epv / 10.0);

    struct failure2Sensor_s {
        HL_FAILURE_FLAG         failureBit;
        MAV_SYS_STATUS_SENSOR   sensorBit;
    };

    static const failure2Sensor_s rgFailure2Sensor[] = {
        { HL_FAILURE_FLAG_GPS,                      MAV_SYS_STATUS_SENSOR_GPS },
        { HL_FAILURE_FLAG_DIFFERENTIAL_PRESSURE,    MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE },
        { HL_FAILURE_FLAG_ABSOLUTE_PRESSURE,        MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE },
        { HL_FAILURE_FLAG_3D_ACCEL,                 MAV_SYS_STATUS_SENSOR_3D_ACCEL },
        { HL_FAILURE_FLAG_3D_GYRO,                  MAV_SYS_STATUS_SENSOR_3D_GYRO },
        { HL_FAILURE_FLAG_3D_MAG,                   MAV_SYS_STATUS_SENSOR_3D_MAG },
    };

    // Map from MAV_FAILURE bits to standard SYS_STATUS message handling
    uint32_t newOnboardControlSensorsEnabled = 0;
    for (size_t i=0; i<sizeof(rgFailure2Sensor)/sizeof(failure2Sensor_s); i++) {
        const failure2Sensor_s* pFailure2Sensor = &rgFailure2Sensor[i];
        if (highLatency2.failure_flags & pFailure2Sensor->failureBit) {
            // Assume if reporting as unhealthy that is it present and enabled
            newOnboardControlSensorsEnabled |= pFailure2Sensor->sensorBit;
        }
    }
    if (newOnboardControlSensorsEnabled != _onboardControlSensorsEnabled) {
        _onboardControlSensorsEnabled = newOnboardControlSensorsEnabled;
        _onboardControlSensorsPresent = newOnboardControlSensorsEnabled;
        _onboardControlSensorsUnhealthy = 0;
        emit unhealthySensorsChanged();
    }
}

void Vehicle::_handleAltitude(mavlink_message_t& message)
{
    mavlink_altitude_t altitude;
    mavlink_msg_altitude_decode(&message, &altitude);

    // If data from GPS is available it takes precedence over ALTITUDE message
    if (!_globalPositionIntMessageAvailable) {
        _altitudeRelativeFact.setRawValue(altitude.altitude_relative);
        if (!_gpsRawIntMessageAvailable) {
            _altitudeAMSLFact.setRawValue(altitude.altitude_amsl);
        }
    }
}

void Vehicle::_setCapabilities(uint64_t capabilityBits)
{
    _capabilityBits = capabilityBits;
    _vehicleCapabilitiesKnown = true;
    emit capabilitiesKnownChanged(true);
    emit capabilityBitsChanged(_capabilityBits);

    // This should potentially be turned into a user-facing warning
    // if the general experience after deployment is that users want MAVLink 2
    // but forget to upgrade their radio firmware
    if (capabilityBits & MAV_PROTOCOL_CAPABILITY_MAVLINK2 && maxProtoVersion() < 200) {
        qCDebug(VehicleLog) << QString("Vehicle does support MAVLink 2 but the link does not allow for it.");
    }

    QString supports("supports");
    QString doesNotSupport("does not support");

    qCDebug(VehicleLog) << QString("Vehicle %1 Mavlink 2.0").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MAVLINK2 ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 MISSION_ITEM_INT").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_INT ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 MISSION_COMMAND_INT").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_COMMAND_INT ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 GeoFence").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_FENCE ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 RallyPoints").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_RALLY ? supports : doesNotSupport);
}

void Vehicle::_handleAutopilotVersion(LinkInterface *link, mavlink_message_t& message)
{
    Q_UNUSED(link);

    mavlink_autopilot_version_t autopilotVersion;
    mavlink_msg_autopilot_version_decode(&message, &autopilotVersion);

    _uid = (quint64)autopilotVersion.uid;
    emit vehicleUIDChanged();

    if (autopilotVersion.flight_sw_version != 0) {
        int majorVersion, minorVersion, patchVersion;
        FIRMWARE_VERSION_TYPE versionType;

        majorVersion = (autopilotVersion.flight_sw_version >> (8*3)) & 0xFF;
        minorVersion = (autopilotVersion.flight_sw_version >> (8*2)) & 0xFF;
        patchVersion = (autopilotVersion.flight_sw_version >> (8*1)) & 0xFF;
        versionType = (FIRMWARE_VERSION_TYPE)((autopilotVersion.flight_sw_version >> (8*0)) & 0xFF);
        setFirmwareVersion(majorVersion, minorVersion, patchVersion, versionType);
    }

    if (px4Firmware()) {
        // Lower 3 bytes is custom version
        int majorVersion, minorVersion, patchVersion;
        majorVersion = autopilotVersion.flight_custom_version[2];
        minorVersion = autopilotVersion.flight_custom_version[1];
        patchVersion = autopilotVersion.flight_custom_version[0];
        setFirmwareCustomVersion(majorVersion, minorVersion, patchVersion);

        // PX4 Firmware stores the first 16 characters of the git hash as binary, with the individual bytes in reverse order
        _gitHash = "";
        QByteArray array((char*)autopilotVersion.flight_custom_version, 8);
        for (int i = 7; i >= 0; i--) {
            _gitHash.append(QString("%1").arg(autopilotVersion.flight_custom_version[i], 2, 16, QChar('0')));
        }
    } else {
        // APM Firmware stores the first 8 characters of the git hash as an ASCII character string
        _gitHash = QString::fromUtf8((char*)autopilotVersion.flight_custom_version, 8);
    }
    emit gitHashChanged(_gitHash);

    _setCapabilities(autopilotVersion.capabilities);
    _startPlanRequest();
}

void Vehicle::_handleProtocolVersion(LinkInterface *link, mavlink_message_t& message)
{
    Q_UNUSED(link);

    mavlink_protocol_version_t protoVersion;
    mavlink_msg_protocol_version_decode(&message, &protoVersion);

    _setMaxProtoVersion(protoVersion.max_version);
}

void Vehicle::_setMaxProtoVersion(unsigned version) {

    // Set only once or if we need to reduce the max version
    if (_maxProtoVersion == 0 || version < _maxProtoVersion) {
        qCDebug(VehicleLog) << "_setMaxProtoVersion before:after" << _maxProtoVersion << version;
        _maxProtoVersion = version;
        emit requestProtocolVersion(_maxProtoVersion);

        // Now that the protocol version is known, the mission load is safe
        // as it will use the right MAVLink version to enable all features
        // the vehicle supports
        _startPlanRequest();
    }
}

QString Vehicle::vehicleUIDStr()
{
    QString uid;
    uint8_t* pUid = (uint8_t*)(void*)&_uid;
    uid.sprintf("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                pUid[0] & 0xff,
            pUid[1] & 0xff,
            pUid[2] & 0xff,
            pUid[3] & 0xff,
            pUid[4] & 0xff,
            pUid[5] & 0xff,
            pUid[6] & 0xff,
            pUid[7] & 0xff);
    return uid;
}

void Vehicle::_handleHilActuatorControls(mavlink_message_t &message)
{
    mavlink_hil_actuator_controls_t hil;
    mavlink_msg_hil_actuator_controls_decode(&message, &hil);
    emit hilActuatorControlsChanged(hil.time_usec, hil.flags,
                                    hil.controls[0],
            hil.controls[1],
            hil.controls[2],
            hil.controls[3],
            hil.controls[4],
            hil.controls[5],
            hil.controls[6],
            hil.controls[7],
            hil.controls[8],
            hil.controls[9],
            hil.controls[10],
            hil.controls[11],
            hil.controls[12],
            hil.controls[13],
            hil.controls[14],
            hil.controls[15],
            hil.mode);
}

void Vehicle::_handleCommandLong(mavlink_message_t& message)
{
#ifdef NO_SERIAL_LINK
    // If not using serial link, bail out.
    Q_UNUSED(message)
#else
    mavlink_command_long_t cmd;
    mavlink_msg_command_long_decode(&message, &cmd);

    switch (cmd.command) {
    // Other component on the same system
    // requests that QGC frees up the serial port of the autopilot
    case MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN:
        if (cmd.param6 > 0) {
            // disconnect the USB link if its a direct connection to a Pixhawk
            for (int i = 0; i < _links.length(); i++) {
                SerialLink *sl = qobject_cast<SerialLink*>(_links.at(i));
                if (sl && sl->getSerialConfig()->usbDirect()) {
                    qDebug() << "Disconnecting:" << _links.at(i)->getName();
                    qgcApp()->toolbox()->linkManager()->disconnectLink(_links.at(i));
                    emit linksChanged();
                }
            }
        }
        break;
    }
#endif
}

void Vehicle::_handleExtendedSysState(mavlink_message_t& message)
{
    mavlink_extended_sys_state_t extendedState;
    mavlink_msg_extended_sys_state_decode(&message, &extendedState);

    switch (extendedState.landed_state) {
    case MAV_LANDED_STATE_ON_GROUND:
        _setFlying(false);
        _setLanding(false);
        break;
    case MAV_LANDED_STATE_TAKEOFF:
    case MAV_LANDED_STATE_IN_AIR:
        _setFlying(true);
        _setLanding(false);
        break;
    case MAV_LANDED_STATE_LANDING:
        _setFlying(true);
        _setLanding(true);
        break;
    default:
        break;
    }

    if (vtol()) {
        bool vtolInFwdFlight = extendedState.vtol_state == MAV_VTOL_STATE_FW;
        if (vtolInFwdFlight != _vtolInFwdFlight) {
            _vtolInFwdFlight = vtolInFwdFlight;
            emit vtolInFwdFlightChanged(vtolInFwdFlight);
        }
    }
}

void Vehicle::_handleVibration(mavlink_message_t& message)
{
    mavlink_vibration_t vibration;
    mavlink_msg_vibration_decode(&message, &vibration);

    _vibrationFactGroup.xAxis()->setRawValue(vibration.vibration_x);
    _vibrationFactGroup.yAxis()->setRawValue(vibration.vibration_y);
    _vibrationFactGroup.zAxis()->setRawValue(vibration.vibration_z);
    _vibrationFactGroup.clipCount1()->setRawValue(vibration.clipping_0);
    _vibrationFactGroup.clipCount2()->setRawValue(vibration.clipping_1);
    _vibrationFactGroup.clipCount3()->setRawValue(vibration.clipping_2);
}

void Vehicle::_handleWindCov(mavlink_message_t& message)
{
    mavlink_wind_cov_t wind;
    mavlink_msg_wind_cov_decode(&message, &wind);

    float direction = qRadiansToDegrees(qAtan2(wind.wind_y, wind.wind_x));
    float speed = qSqrt(qPow(wind.wind_x, 2) + qPow(wind.wind_y, 2));

    if (direction < 0) {
        direction += 360;
    }

    _windFactGroup.direction()->setRawValue(direction);
    _windFactGroup.speed()->setRawValue(speed);
    _windFactGroup.verticalSpeed()->setRawValue(0);
}

#if !defined(NO_ARDUPILOT_DIALECT)
void Vehicle::_handleWind(mavlink_message_t& message)
{
    mavlink_wind_t wind;
    mavlink_msg_wind_decode(&message, &wind);

    // We don't want negative wind angles
    float direction = wind.direction;
    if (direction < 0) {
        direction += 360;
    }
    _windFactGroup.direction()->setRawValue(direction);
    _windFactGroup.speed()->setRawValue(wind.speed);
    _windFactGroup.verticalSpeed()->setRawValue(wind.speed_z);
}
#endif

bool Vehicle::_apmArmingNotRequired(void)
{
    QString armingRequireParam("ARMING_REQUIRE");
    return _parameterManager->parameterExists(FactSystem::defaultComponentId, armingRequireParam) &&
            _parameterManager->getParameter(FactSystem::defaultComponentId, armingRequireParam)->rawValue().toInt() == 0;
}

void Vehicle::_handleSysStatus(mavlink_message_t& message)
{
    mavlink_sys_status_t sysStatus;
    mavlink_msg_sys_status_decode(&message, &sysStatus);

    if (sysStatus.current_battery == -1) {
        _battery1FactGroup.current()->setRawValue(VehicleBatteryFactGroup::_currentUnavailable);
    } else {
        // Current is in Amps, current_battery is 10 * milliamperes (1 = 10 milliampere)
        _battery1FactGroup.current()->setRawValue((float)sysStatus.current_battery / 100.0f);
    }
    if (sysStatus.voltage_battery == UINT16_MAX) {
        _battery1FactGroup.voltage()->setRawValue(VehicleBatteryFactGroup::_voltageUnavailable);
    } else {
        _battery1FactGroup.voltage()->setRawValue((double)sysStatus.voltage_battery / 1000.0);
        // current_battery is 10 mA and voltage_battery is 1mV. (10/1e3 times 1/1e3 = 1/1e5)
        _battery1FactGroup.instantPower()->setRawValue((float)(sysStatus.current_battery*sysStatus.voltage_battery)/(100000.0));
    }
    _battery1FactGroup.percentRemaining()->setRawValue(sysStatus.battery_remaining);

    if (sysStatus.battery_remaining > 0) {
        if (sysStatus.battery_remaining < _settingsManager->appSettings()->batteryPercentRemainingAnnounce()->rawValue().toInt() &&
                sysStatus.battery_remaining < _lastAnnouncedLowBatteryPercent) {
            _say(QString(tr("%1 low battery: %2 percent remaining")).arg(_vehicleIdSpeech()).arg(sysStatus.battery_remaining));
        }
        _lastAnnouncedLowBatteryPercent = sysStatus.battery_remaining;
    }

    if (_onboardControlSensorsPresent != sysStatus.onboard_control_sensors_present) {
        _onboardControlSensorsPresent = sysStatus.onboard_control_sensors_present;
        emit sensorsPresentBitsChanged(_onboardControlSensorsPresent);
    }
    if (_onboardControlSensorsEnabled != sysStatus.onboard_control_sensors_enabled) {
        _onboardControlSensorsEnabled = sysStatus.onboard_control_sensors_enabled;
        emit sensorsEnabledBitsChanged(_onboardControlSensorsEnabled);
    }
    if (_onboardControlSensorsHealth != sysStatus.onboard_control_sensors_health) {
        _onboardControlSensorsHealth = sysStatus.onboard_control_sensors_health;
        emit sensorsHealthBitsChanged(_onboardControlSensorsHealth);
    }

    // ArduPilot firmare has a strange case when ARMING_REQUIRE=0. This means the vehicle is always armed but the motors are not
    // really powered up until the safety button is pressed. Because of this we can't depend on the heartbeat to tell us the true
    // armed (and dangerous) state. We must instead rely on SYS_STATUS telling us that the motors are enabled.
    if (apmFirmware() && _apmArmingNotRequired()) {
        _updateArmed(_onboardControlSensorsEnabled & MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS);
    }

    uint32_t newSensorsUnhealthy = _onboardControlSensorsEnabled & ~_onboardControlSensorsHealth;
    if (newSensorsUnhealthy != _onboardControlSensorsUnhealthy) {
        _onboardControlSensorsUnhealthy = newSensorsUnhealthy;
        emit unhealthySensorsChanged();
        emit sensorsUnhealthyBitsChanged(_onboardControlSensorsUnhealthy);
    }
}

void Vehicle::_handleBatteryStatus(mavlink_message_t& message)
{
    mavlink_battery_status_t bat_status;
    mavlink_msg_battery_status_decode(&message, &bat_status);

    VehicleBatteryFactGroup& batteryFactGroup = bat_status.id == 0 ? _battery1FactGroup : _battery2FactGroup;

    if (bat_status.temperature == INT16_MAX) {
        batteryFactGroup.temperature()->setRawValue(VehicleBatteryFactGroup::_temperatureUnavailable);
    } else {
        batteryFactGroup.temperature()->setRawValue((double)bat_status.temperature / 100.0);
    }
    if (bat_status.current_consumed == -1) {
        batteryFactGroup.mahConsumed()->setRawValue(VehicleBatteryFactGroup::_mahConsumedUnavailable);
    } else {
        batteryFactGroup.mahConsumed()->setRawValue(bat_status.current_consumed);
    }

    int cellCount = 0;
    for (int i=0; i<10; i++) {
        if (bat_status.voltages[i] != UINT16_MAX) {
            cellCount++;
        }
    }
    if (cellCount == 0) {
        cellCount = -1;
    }

    batteryFactGroup.cellCount()->setRawValue(cellCount);

    //-- Time remaining in seconds (0 means not supported)
    batteryFactGroup.timeRemaining()->setRawValue(bat_status.time_remaining);
    //-- Battery charge state (0 means not supported)
    if(bat_status.charge_state <= MAV_BATTERY_CHARGE_STATE_UNHEALTHY) {
        batteryFactGroup.chargeState()->setRawValue(bat_status.charge_state);
    } else {
        batteryFactGroup.chargeState()->setRawValue(0);
    }
    //-- TODO: Somewhere, actions would be taken based on this chargeState:
    //   MAV_BATTERY_CHARGE_STATE_CRITICAL:     Battery state is critical, return / abort immediately
    //   MAV_BATTERY_CHARGE_STATE_EMERGENCY:    Battery state is too low for ordinary abortion, fastest possible emergency stop preventing damage
    //   MAV_BATTERY_CHARGE_STATE_FAILED:       Battery failed, damage unavoidable
    //   MAV_BATTERY_CHARGE_STATE_UNHEALTHY:    Battery is diagnosed to be broken or an error occurred, usage is discouraged / prohibited
}

void Vehicle::_setHomePosition(QGeoCoordinate& homeCoord)
{
    if (homeCoord != _homePosition) {
        _homePosition = homeCoord;
        emit homePositionChanged(_homePosition);
    }
}

void Vehicle::_handleHomePosition(mavlink_message_t& message)
{
    mavlink_home_position_t homePos;

    mavlink_msg_home_position_decode(&message, &homePos);

    QGeoCoordinate newHomePosition (homePos.latitude / 10000000.0,
                                    homePos.longitude / 10000000.0,
                                    homePos.altitude / 1000.0);
    _setHomePosition(newHomePosition);
}

void Vehicle::_updateArmed(bool armed)
{
    if (_armed != armed) {
        _armed = armed;
        emit armedChanged(_armed);

        // We are transitioning to the armed state, begin tracking trajectory points for the map
        if (_armed) {
            _mapTrajectoryStart();
            _clearCameraTriggerPoints();
        } else {
            _mapTrajectoryStop();
            // Also handle Video Streaming
            if(_settingsManager->videoSettings()->disableWhenDisarmed()->rawValue().toBool()) {
                _settingsManager->videoSettings()->streamEnabled()->setRawValue(false);
                qgcApp()->toolbox()->videoManager()->videoReceiver()->stop();
            }
        }
    }
}

void Vehicle::_handlePing(LinkInterface* link, mavlink_message_t& message)
{
    mavlink_ping_t      ping;
    mavlink_message_t   msg;

    mavlink_msg_ping_decode(&message, &ping);
    mavlink_msg_ping_pack_chan(_mavlink->getSystemId(),
                               _mavlink->getComponentId(),
                               priorityLink()->mavlinkChannel(),
                               &msg,
                               ping.time_usec,
                               ping.seq,
                               message.sysid,
                               message.compid);
    sendMessageOnLink(link, msg);
}

void Vehicle::_handleHeartbeat(mavlink_message_t& message)
{
    if (message.compid != _defaultComponentId) {
        return;
    }

    mavlink_heartbeat_t heartbeat;

    mavlink_msg_heartbeat_decode(&message, &heartbeat);

    bool newArmed = heartbeat.base_mode & MAV_MODE_FLAG_DECODE_POSITION_SAFETY;

    // ArduPilot firmare has a strange case when ARMING_REQUIRE=0. This means the vehicle is always armed but the motors are not
    // really powered up until the safety button is pressed. Because of this we can't depend on the heartbeat to tell us the true
    // armed (and dangerous) state. We must instead rely on SYS_STATUS telling us that the motors are enabled.
    if (apmFirmware()) {
        if (!_apmArmingNotRequired() || !(_onboardControlSensorsPresent & MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS)) {
            // If ARMING_REQUIRE!=0 or we haven't seen motor output status yet we use the hearbeat info for armed
            _updateArmed(newArmed);
        }
    } else {
        // Non-ArduPilot always updates from armed state in heartbeat
        _updateArmed(newArmed);
    }

    if (heartbeat.base_mode != _base_mode || heartbeat.custom_mode != _custom_mode) {
        QString previousFlightMode;
        if (_base_mode != 0 || _custom_mode != 0){
            // Vehicle is initialized with _base_mode=0 and _custom_mode=0. Don't pass this to flightMode() since it will complain about
            // bad modes while unit testing.
            previousFlightMode = flightMode();
        }
        _base_mode = heartbeat.base_mode;
        _custom_mode = heartbeat.custom_mode;
        if (previousFlightMode != flightMode()) {
            emit flightModeChanged(flightMode());
        }
    }
}

void Vehicle::_handleRadioStatus(mavlink_message_t& message)
{

    //-- Process telemetry status message
    mavlink_radio_status_t rstatus;
    mavlink_msg_radio_status_decode(&message, &rstatus);

    int rssi    = rstatus.rssi;
    int remrssi = rstatus.remrssi;
    int lnoise = (int)(int8_t)rstatus.noise;
    int rnoise = (int)(int8_t)rstatus.remnoise;
    //-- 3DR Si1k radio needs rssi fields to be converted to dBm
    if (message.sysid == '3' && message.compid == 'D') {
        /* Per the Si1K datasheet figure 23.25 and SI AN474 code
         * samples the relationship between the RSSI register
         * and received power is as follows:
         *
         *                       10
         * inputPower = rssi * ------ 127
         *                       19
         *
         * Additionally limit to the only realistic range [-120,0] dBm
         */
        rssi    = qMin(qMax(qRound(static_cast<qreal>(rssi)    / 1.9 - 127.0), - 120), 0);
        remrssi = qMin(qMax(qRound(static_cast<qreal>(remrssi) / 1.9 - 127.0), - 120), 0);
    } else {
        rssi    = (int)(int8_t)rstatus.rssi;
        remrssi = (int)(int8_t)rstatus.remrssi;
    }
    //-- Check for changes
    if(_telemetryLRSSI != rssi) {
        _telemetryLRSSI = rssi;
        emit telemetryLRSSIChanged(_telemetryLRSSI);
    }
    if(_telemetryRRSSI != remrssi) {
        _telemetryRRSSI = remrssi;
        emit telemetryRRSSIChanged(_telemetryRRSSI);
    }
    if(_telemetryRXErrors != rstatus.rxerrors) {
        _telemetryRXErrors = rstatus.rxerrors;
        emit telemetryRXErrorsChanged(_telemetryRXErrors);
    }
    if(_telemetryFixed != rstatus.fixed) {
        _telemetryFixed = rstatus.fixed;
        emit telemetryFixedChanged(_telemetryFixed);
    }
    if(_telemetryTXBuffer != rstatus.txbuf) {
        _telemetryTXBuffer = rstatus.txbuf;
        emit telemetryTXBufferChanged(_telemetryTXBuffer);
    }
    if(_telemetryLNoise != lnoise) {
        _telemetryLNoise = lnoise;
        emit telemetryLNoiseChanged(_telemetryLNoise);
    }
    if(_telemetryRNoise != rnoise) {
        _telemetryRNoise = rnoise;
        emit telemetryRNoiseChanged(_telemetryRNoise);
    }
}

void Vehicle::_handleRCChannels(mavlink_message_t& message)
{
    mavlink_rc_channels_t channels;

    mavlink_msg_rc_channels_decode(&message, &channels);

    uint16_t* _rgChannelvalues[cMaxRcChannels] = {
        &channels.chan1_raw,
        &channels.chan2_raw,
        &channels.chan3_raw,
        &channels.chan4_raw,
        &channels.chan5_raw,
        &channels.chan6_raw,
        &channels.chan7_raw,
        &channels.chan8_raw,
        &channels.chan9_raw,
        &channels.chan10_raw,
        &channels.chan11_raw,
        &channels.chan12_raw,
        &channels.chan13_raw,
        &channels.chan14_raw,
        &channels.chan15_raw,
        &channels.chan16_raw,
        &channels.chan17_raw,
        &channels.chan18_raw,
    };
    int pwmValues[cMaxRcChannels];

    for (int i=0; i<cMaxRcChannels; i++) {
        uint16_t channelValue = *_rgChannelvalues[i];

        if (i < channels.chancount) {
            pwmValues[i] = channelValue == UINT16_MAX ? -1 : channelValue;
        } else {
            pwmValues[i] = -1;
        }
    }

    emit remoteControlRSSIChanged(channels.rssi);
    emit rcChannelsChanged(channels.chancount, pwmValues);
}

void Vehicle::_handleRCChannelsRaw(mavlink_message_t& message)
{
    // We handle both RC_CHANNLES and RC_CHANNELS_RAW since different firmware will only
    // send one or the other.

    mavlink_rc_channels_raw_t channels;

    mavlink_msg_rc_channels_raw_decode(&message, &channels);

    uint16_t* _rgChannelvalues[cMaxRcChannels] = {
        &channels.chan1_raw,
        &channels.chan2_raw,
        &channels.chan3_raw,
        &channels.chan4_raw,
        &channels.chan5_raw,
        &channels.chan6_raw,
        &channels.chan7_raw,
        &channels.chan8_raw,
    };

    int pwmValues[cMaxRcChannels];
    int channelCount = 0;

    for (int i=0; i<cMaxRcChannels; i++) {
        pwmValues[i] = -1;
    }

    for (int i=0; i<8; i++) {
        uint16_t channelValue = *_rgChannelvalues[i];

        if (channelValue == UINT16_MAX) {
            pwmValues[i] = -1;
        } else {
            channelCount = i + 1;
            pwmValues[i] = channelValue;
        }
    }
    for (int i=9; i<18; i++) {
        pwmValues[i] = -1;
    }

    emit remoteControlRSSIChanged(channels.rssi);
    emit rcChannelsChanged(channelCount, pwmValues);
}

void Vehicle::_handleScaledPressure(mavlink_message_t& message) {
    mavlink_scaled_pressure_t pressure;
    mavlink_msg_scaled_pressure_decode(&message, &pressure);
    _temperatureFactGroup.temperature1()->setRawValue(pressure.temperature / 100.0);
}

void Vehicle::_handleScaledPressure2(mavlink_message_t& message) {
    mavlink_scaled_pressure2_t pressure;
    mavlink_msg_scaled_pressure2_decode(&message, &pressure);
    _temperatureFactGroup.temperature2()->setRawValue(pressure.temperature / 100.0);
}

void Vehicle::_handleScaledPressure3(mavlink_message_t& message) {
    mavlink_scaled_pressure3_t pressure;
    mavlink_msg_scaled_pressure3_decode(&message, &pressure);
    _temperatureFactGroup.temperature3()->setRawValue(pressure.temperature / 100.0);
}

bool Vehicle::_containsLink(LinkInterface* link)
{
    return _links.contains(link);
}

void Vehicle::_addLink(LinkInterface* link)
{
    if (!_containsLink(link)) {
        qCDebug(VehicleLog) << "_addLink:" << QString("%1").arg((ulong)link, 0, 16);
        _links += link;
        if (_links.count() <= 1) {
            _updatePriorityLink(true /* updateActive */, false /* sendCommand */);
        } else {
            _updatePriorityLink(true /* updateActive */, true /* sendCommand */);
        }

        connect(_toolbox->linkManager(), &LinkManager::linkInactive, this, &Vehicle::_linkInactiveOrDeleted);
        connect(_toolbox->linkManager(), &LinkManager::linkDeleted, this, &Vehicle::_linkInactiveOrDeleted);
        connect(link, &LinkInterface::highLatencyChanged, this, &Vehicle::_updateHighLatencyLink);
        connect(link, &LinkInterface::activeChanged, this, &Vehicle::_linkActiveChanged);
    }
}

void Vehicle::_linkInactiveOrDeleted(LinkInterface* link)
{
    qCDebug(VehicleLog) << "_linkInactiveOrDeleted linkCount" << _links.count();

    _links.removeOne(link);

    if (_priorityLink.data() == link) {
        _priorityLink.clear();
    }

    _updatePriorityLink(true /* updateActive */, true /* sendCommand */);

    if (_links.count() == 0 && !_allLinksInactiveSent) {
        qCDebug(VehicleLog) << "All links inactive";
        // Make sure to not send this more than one time
        _allLinksInactiveSent = true;
        emit allLinksInactive(this);
    }
}

bool Vehicle::sendMessageOnLink(LinkInterface* link, mavlink_message_t message)
{
    if (!link || !_links.contains(link) || !link->isConnected()) {
        return false;
    }

    emit _sendMessageOnLinkOnThread(link, message);

    return true;
}

void Vehicle::_sendMessageOnLink(LinkInterface* link, mavlink_message_t message)
{
    // Make sure this is still a good link
    if (!link || !_links.contains(link) || !link->isConnected()) {
        return;
    }

#if 0
    // Leaving in for ease in Mav 2.0 testing
    mavlink_status_t* mavlinkStatus = mavlink_get_channel_status(link->mavlinkChannel());
    qDebug() << "_sendMessageOnLink" << mavlinkStatus << link->mavlinkChannel() << mavlinkStatus->flags << message.magic;
#endif

    // Give the plugin a chance to adjust
    _firmwarePlugin->adjustOutgoingMavlinkMessage(this, link, &message);

    // Write message into buffer, prepending start sign
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buffer, &message);

    link->writeBytesSafe((const char*)buffer, len);
    _messagesSent++;
    emit messagesSentChanged();
}

void Vehicle::_updatePriorityLink(bool updateActive, bool sendCommand)
{
    emit linksPropertiesChanged();

    // if the priority link is commanded and still active don't change anything
    if (_priorityLinkCommanded) {
        if (_priorityLink.data()->link_active(_id)) {
            return;
        } else {
            _priorityLinkCommanded = false;
        }
    }

    LinkInterface* newPriorityLink = NULL;

    // This routine specifically does not clear _priorityLink when there are no links remaining.
    // By doing this we hold a reference on the last link as the Vehicle shuts down. Thus preventing shutdown
    // ordering NULL pointer crashes where priorityLink() is still called during shutdown sequence.
    if (_links.count() == 0) {
        return;
    }

    // Check for the existing priority link to still be valid
    for (int i=0; i<_links.count(); i++) {
        if (_priorityLink.data() == _links[i]) {
            if (!_priorityLink.data()->highLatency() && _priorityLink->link_active(_id)) {
                // Link is still valid. Continue to use it unless it is high latency. In that case we still look for a better
                // link to use as priority link.
                return;
            }
        }
    }

    // The previous priority link is no longer valid. We must no find the best link available in this priority order:
    //      First active direct USB connection
    //      Any active non high latency link
    //      An active high latency link
    //      Any link

#ifndef NO_SERIAL_LINK
    // Search for an active direct usb connection
    for (int i=0; i<_links.count(); i++) {
        LinkInterface* link = _links[i];
        SerialLink* pSerialLink = qobject_cast<SerialLink*>(link);
        if (pSerialLink) {
            LinkConfiguration* config = pSerialLink->getLinkConfiguration();
            if (config) {
                SerialConfiguration* pSerialConfig = qobject_cast<SerialConfiguration*>(config);
                if (pSerialConfig && pSerialConfig->usbDirect()) {
                    if (_priorityLink.data() != link && link->link_active(_id)) {
                        newPriorityLink = link;
                        break;
                    }
                }
            }
        }
    }
#endif

    if (!newPriorityLink) {
        // Search for an active non-high latency link
        for (int i=0; i<_links.count(); i++) {
            LinkInterface* link = _links[i];
            if (!link->highLatency() && link->link_active(_id)) {
                newPriorityLink = link;
                break;
            }
        }
    }

    if (!newPriorityLink) {
        // Search for an active high latency link
        for (int i=0; i<_links.count(); i++) {
            LinkInterface* link = _links[i];
            if (link->highLatency() && link->link_active(_id)) {
                newPriorityLink = link;
                break;
            }
        }
    }

    if (!newPriorityLink) {
        // Use any link
        newPriorityLink = _links[0];
    }

    if (_priorityLink.data() != newPriorityLink) {
        if (_priorityLink) {
            qgcApp()->showMessage((tr("switch to %2 as priority link")).arg(newPriorityLink->getName()));
        }
        _priorityLink = _toolbox->linkManager()->sharedLinkInterfacePointerForLink(newPriorityLink);

        _updateHighLatencyLink(sendCommand);

        emit priorityLinkNameChanged(_priorityLink->getName());
        if (updateActive) {
            _linkActiveChanged(_priorityLink.data(), _priorityLink->link_active(_id), _id);
        }
    }
}

int Vehicle::motorCount(void)
{
    switch (_vehicleType) {
    case MAV_TYPE_HELICOPTER:
        return 1;
    case MAV_TYPE_VTOL_DUOROTOR:
        return 2;
    case MAV_TYPE_TRICOPTER:
        return 3;
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_VTOL_QUADROTOR:
        return 4;
    case MAV_TYPE_HEXAROTOR:
        return 6;
    case MAV_TYPE_OCTOROTOR:
        return 8;
    default:
        return -1;
    }
}

bool Vehicle::coaxialMotors(void)
{
    return _firmwarePlugin->multiRotorCoaxialMotors(this);
}

bool Vehicle::xConfigMotors(void)
{
    return _firmwarePlugin->multiRotorXConfig(this);
}

QString Vehicle::formatedMessages()
{
    QString messages;
    foreach(UASMessage* message, _toolbox->uasMessageHandler()->messages()) {
        messages += message->getFormatedText();
    }
    return messages;
}

void Vehicle::clearMessages()
{
    _toolbox->uasMessageHandler()->clearMessages();
}

void Vehicle::_handletextMessageReceived(UASMessage* message)
{
    if(message)
    {
        _formatedMessage = message->getFormatedText();
        emit formatedMessageChanged();
    }
}

void Vehicle::_handleTextMessage(int newCount)
{
    // Reset?
    if(!newCount) {
        _currentMessageCount = 0;
        _currentNormalCount  = 0;
        _currentWarningCount = 0;
        _currentErrorCount   = 0;
        _messageCount        = 0;
        _currentMessageType  = MessageNone;
        emit newMessageCountChanged();
        emit messageTypeChanged();
        emit messageCountChanged();
        return;
    }

    UASMessageHandler* pMh = _toolbox->uasMessageHandler();
    MessageType_t type = newCount ? _currentMessageType : MessageNone;
    int errorCount     = _currentErrorCount;
    int warnCount      = _currentWarningCount;
    int normalCount    = _currentNormalCount;
    //-- Add current message counts
    errorCount  += pMh->getErrorCount();
    warnCount   += pMh->getWarningCount();
    normalCount += pMh->getNormalCount();
    //-- See if we have a higher level
    if(errorCount != _currentErrorCount) {
        _currentErrorCount = errorCount;
        type = MessageError;
    }
    if(warnCount != _currentWarningCount) {
        _currentWarningCount = warnCount;
        if(_currentMessageType != MessageError) {
            type = MessageWarning;
        }
    }
    if(normalCount != _currentNormalCount) {
        _currentNormalCount = normalCount;
        if(_currentMessageType != MessageError && _currentMessageType != MessageWarning) {
            type = MessageNormal;
        }
    }
    int count = _currentErrorCount + _currentWarningCount + _currentNormalCount;
    if(count != _currentMessageCount) {
        _currentMessageCount = count;
        // Display current total new messages count
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        _currentMessageType = type;
        // Update message level
        emit messageTypeChanged();
    }
    // Update message count (all messages)
    if(newCount != _messageCount) {
        _messageCount = newCount;
        emit messageCountChanged();
    }
    QString errMsg = pMh->getLatestError();
    if(errMsg != _latestError) {
        _latestError = errMsg;
        emit latestErrorChanged();
    }
}

void Vehicle::resetMessages()
{
    // Reset Counts
    int count = _currentMessageCount;
    MessageType_t type = _currentMessageType;
    _currentErrorCount   = 0;
    _currentWarningCount = 0;
    _currentNormalCount  = 0;
    _currentMessageCount = 0;
    _currentMessageType = MessageNone;
    if(count != _currentMessageCount) {
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        emit messageTypeChanged();
    }
}

int Vehicle::manualControlReservedButtonCount(void)
{
    return _firmwarePlugin->manualControlReservedButtonCount();
}

void Vehicle::_loadSettings(void)
{
    if (!_active) {
        return;
    }

    QSettings settings;

    settings.beginGroup(QString(_settingsGroup).arg(_id));

    bool convertOk;

    _joystickMode = (JoystickMode_t)settings.value(_joystickModeSettingsKey, JoystickModeRC).toInt(&convertOk);
    if (!convertOk) {
        _joystickMode = JoystickModeRC;
    }

    // Joystick enabled is a global setting so first make sure there are any joysticks connected
    if (_toolbox->joystickManager()->joysticks().count()) {
        setJoystickEnabled(settings.value(_joystickEnabledSettingsKey, false).toBool());
        _startJoystick(true);
    }
}

void Vehicle::_saveSettings(void)
{
    QSettings settings;

    settings.beginGroup(QString(_settingsGroup).arg(_id));

    settings.setValue(_joystickModeSettingsKey, _joystickMode);

    // The joystick enabled setting should only be changed if a joystick is present
    // since the checkbox can only be clicked if one is present
    if (_toolbox->joystickManager()->joysticks().count()) {
        settings.setValue(_joystickEnabledSettingsKey, _joystickEnabled);
    }
}

int Vehicle::joystickMode(void)
{
    return _joystickMode;
}

void Vehicle::setJoystickMode(int mode)
{
    if (mode < 0 || mode >= JoystickModeMax) {
        qCWarning(VehicleLog) << "Invalid joystick mode" << mode;
        return;
    }

    _joystickMode = (JoystickMode_t)mode;
    _saveSettings();
    emit joystickModeChanged(mode);
}

QStringList Vehicle::joystickModes(void)
{
    QStringList list;

    list << "Normal" << "Attitude" << "Position" << "Force" << "Velocity";

    return list;
}

bool Vehicle::joystickEnabled(void)
{
    return _joystickEnabled;
}

void Vehicle::setJoystickEnabled(bool enabled)
{
    _joystickEnabled = enabled;
    _saveSettings();
    emit joystickEnabledChanged(_joystickEnabled);
}

void Vehicle::_startJoystick(bool start)
{
    Joystick* joystick = _joystickManager->activeJoystick();
    if (joystick) {
        if (start) {
            joystick->startPolling(this);
        } else {
            joystick->stopPolling();
        }
    }
}

bool Vehicle::active(void)
{
    return _active;
}

void Vehicle::setActive(bool active)
{
    if (_active != active) {
        _active = active;
        _startJoystick(false);
        emit activeChanged(_active);
    }
}

QGeoCoordinate Vehicle::homePosition(void)
{
    return _homePosition;
}

void Vehicle::setArmed(bool armed)
{
    // We specifically use COMMAND_LONG:MAV_CMD_COMPONENT_ARM_DISARM since it is supported by more flight stacks.
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_COMPONENT_ARM_DISARM,
                   true,    // show error if fails
                   armed ? 1.0f : 0.0f);
}

bool Vehicle::flightModeSetAvailable(void)
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::SetFlightModeCapability);
}

QStringList Vehicle::flightModes(void)
{
    return _firmwarePlugin->flightModes(this);
}

QString Vehicle::flightMode(void) const
{
    return _firmwarePlugin->flightMode(_base_mode, _custom_mode);
}

void Vehicle::setFlightMode(const QString& flightMode)
{
    uint8_t     base_mode;
    uint32_t    custom_mode;

    if (_firmwarePlugin->setFlightMode(flightMode, &base_mode, &custom_mode)) {
        // setFlightMode will only set MAV_MODE_FLAG_CUSTOM_MODE_ENABLED in base_mode, we need to move back in the existing
        // states.
        uint8_t newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_CUSTOM_MODE;
        newBaseMode |= base_mode;

        mavlink_message_t msg;
        mavlink_msg_set_mode_pack_chan(_mavlink->getSystemId(),
                                       _mavlink->getComponentId(),
                                       priorityLink()->mavlinkChannel(),
                                       &msg,
                                       id(),
                                       newBaseMode,
                                       custom_mode);
        sendMessageOnLink(priorityLink(), msg);
    } else {
        qWarning() << "FirmwarePlugin::setFlightMode failed, flightMode:" << flightMode;
    }
}

QString Vehicle::priorityLinkName(void) const
{
    if (_priorityLink) {
        return _priorityLink->getName();
    }

    return "none";
}

QVariantList Vehicle::links(void) const {
    QVariantList ret;

    foreach( const auto &item, _links )
        ret << QVariant::fromValue(item);

    return ret;
}

void Vehicle::setPriorityLinkByName(const QString& priorityLinkName)
{
    if (!_priorityLink) {
        return;
    }

    if (priorityLinkName == _priorityLink->getName()) {
        // The link did not change
        return;
    }

    LinkInterface* newPriorityLink = NULL;


    for (int i=0; i<_links.count(); i++) {
        if (_links[i]->getName() == priorityLinkName) {
            newPriorityLink = _links[i];
        }
    }

    if (newPriorityLink) {
        _priorityLinkCommanded = true;
        _priorityLink = _toolbox->linkManager()->sharedLinkInterfacePointerForLink(newPriorityLink);
        _updateHighLatencyLink(true);
        emit priorityLinkNameChanged(_priorityLink->getName());
        _linkActiveChanged(_priorityLink.data(), _priorityLink->link_active(_id), _id);
    }
}

bool Vehicle::hilMode(void)
{
    return _base_mode & MAV_MODE_FLAG_HIL_ENABLED;
}

void Vehicle::setHilMode(bool hilMode)
{
    mavlink_message_t msg;

    uint8_t newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_HIL;
    if (hilMode) {
        newBaseMode |= MAV_MODE_FLAG_HIL_ENABLED;
    }

    mavlink_msg_set_mode_pack_chan(_mavlink->getSystemId(),
                                   _mavlink->getComponentId(),
                                   priorityLink()->mavlinkChannel(),
                                   &msg,
                                   id(),
                                   newBaseMode,
                                   _custom_mode);
    sendMessageOnLink(priorityLink(), msg);
}

void Vehicle::requestDataStream(MAV_DATA_STREAM stream, uint16_t rate, bool sendMultiple)
{
    mavlink_message_t               msg;
    mavlink_request_data_stream_t   dataStream;

    memset(&dataStream, 0, sizeof(dataStream));

    dataStream.req_stream_id = stream;
    dataStream.req_message_rate = rate;
    dataStream.start_stop = 1;  // start
    dataStream.target_system = id();
    dataStream.target_component = _defaultComponentId;

    mavlink_msg_request_data_stream_encode_chan(_mavlink->getSystemId(),
                                                _mavlink->getComponentId(),
                                                priorityLink()->mavlinkChannel(),
                                                &msg,
                                                &dataStream);

    if (sendMultiple) {
        // We use sendMessageMultiple since we really want these to make it to the vehicle
        sendMessageMultiple(msg);
    } else {
        sendMessageOnLink(priorityLink(), msg);
    }
}

void Vehicle::_sendMessageMultipleNext(void)
{
    if (_nextSendMessageMultipleIndex < _sendMessageMultipleList.count()) {
        qCDebug(VehicleLog) << "_sendMessageMultipleNext:" << _sendMessageMultipleList[_nextSendMessageMultipleIndex].message.msgid;

        sendMessageOnLink(priorityLink(), _sendMessageMultipleList[_nextSendMessageMultipleIndex].message);

        if (--_sendMessageMultipleList[_nextSendMessageMultipleIndex].retryCount <= 0) {
            _sendMessageMultipleList.removeAt(_nextSendMessageMultipleIndex);
        } else {
            _nextSendMessageMultipleIndex++;
        }
    }

    if (_nextSendMessageMultipleIndex >= _sendMessageMultipleList.count()) {
        _nextSendMessageMultipleIndex = 0;
    }
}

void Vehicle::sendMessageMultiple(mavlink_message_t message)
{
    SendMessageMultipleInfo_t   info;

    info.message =      message;
    info.retryCount =   _sendMessageMultipleRetries;

    _sendMessageMultipleList.append(info);
}

void Vehicle::_missionManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showMessage(tr("Mission transfer failed. Retry transfer. Error: %1").arg(errorMsg));
}

void Vehicle::_geoFenceManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showMessage(tr("GeoFence transfer failed. Retry transfer. Error: %1").arg(errorMsg));
}

void Vehicle::_rallyPointManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showMessage(tr("Rally Point transfer failed. Retry transfer. Error: %1").arg(errorMsg));
}

void Vehicle::_addNewMapTrajectoryPoint(void)
{
    if (_mapTrajectoryHaveFirstCoordinate) {
        // Keep three minutes of trajectory on mobile due to perf impact of lines
#ifdef __mobile__
        if (_mapTrajectoryList.count() * _mapTrajectoryMsecsBetweenPoints > 3 * 1000 * 60) {
            _mapTrajectoryList.removeAt(0)->deleteLater();
        }
#endif
        _mapTrajectoryList.append(new CoordinateVector(_mapTrajectoryLastCoordinate, _coordinate, this));
        _flightDistanceFact.setRawValue(_flightDistanceFact.rawValue().toDouble() + _mapTrajectoryLastCoordinate.distanceTo(_coordinate));
    }
    _mapTrajectoryHaveFirstCoordinate = true;
    _mapTrajectoryLastCoordinate = _coordinate;
    _flightTimeFact.setRawValue((double)_flightTimer.elapsed() / 1000.0);
}

void Vehicle::_clearTrajectoryPoints(void)
{
    _mapTrajectoryList.clearAndDeleteContents();
}

void Vehicle::_clearCameraTriggerPoints(void)
{
    _cameraTriggerPoints.clearAndDeleteContents();
}

void Vehicle::_mapTrajectoryStart(void)
{
    _mapTrajectoryHaveFirstCoordinate = false;
    _clearTrajectoryPoints();
    _mapTrajectoryTimer.start();
    _flightTimer.start();
    _flightDistanceFact.setRawValue(0);
    _flightTimeFact.setRawValue(0);
}

void Vehicle::_mapTrajectoryStop()
{
    _mapTrajectoryTimer.stop();
}

void Vehicle::_startPlanRequest(void)
{
    if (_missionManagerInitialRequestSent) {
        return;
    }

    if (_parameterManager->parametersReady() && _vehicleCapabilitiesKnown) {
        qCDebug(VehicleLog) << "_startPlanRequest";
        _missionManagerInitialRequestSent = true;
        if (_settingsManager->appSettings()->autoLoadMissions()->rawValue().toBool()) {
            QString missionAutoLoadDirPath = _settingsManager->appSettings()->missionSavePath();
            if (!missionAutoLoadDirPath.isEmpty()) {
                QDir missionAutoLoadDir(missionAutoLoadDirPath);
                QString autoloadFilename = missionAutoLoadDir.absoluteFilePath(tr("AutoLoad%1.%2").arg(_id).arg(AppSettings::planFileExtension));
                if (QFile(autoloadFilename).exists()) {
                    _initialPlanRequestComplete = true; // We aren't going to load from the vehicle, so we are done
                    PlanMasterController::sendPlanToVehicle(this, autoloadFilename);
                    return;
                }
            }
        }
        _missionManager->loadFromVehicle();
    } else {
        if (!_parameterManager->parametersReady()) {
            qCDebug(VehicleLog) << "Delaying _startPlanRequest due to parameters not ready";
        } else if (!_vehicleCapabilitiesKnown) {
            qCDebug(VehicleLog) << "Delaying _startPlanRequest due to vehicle capabilities not known";
        }
    }
}

void Vehicle::_missionLoadComplete(void)
{
    // After the initial mission request completes we ask for the geofence
    if (!_geoFenceManagerInitialRequestSent) {
        _geoFenceManagerInitialRequestSent = true;
        if (_geoFenceManager->supported()) {
            qCDebug(VehicleLog) << "_missionLoadComplete requesting GeoFence";
            _geoFenceManager->loadFromVehicle();
        } else {
            qCDebug(VehicleLog) << "_missionLoadComplete GeoFence not supported skipping";
            _geoFenceLoadComplete();
        }
    }
}

void Vehicle::_geoFenceLoadComplete(void)
{
    // After geofence request completes we ask for the rally points
    if (!_rallyPointManagerInitialRequestSent) {
        _rallyPointManagerInitialRequestSent = true;
        if (_rallyPointManager->supported()) {
            qCDebug(VehicleLog) << "_missionLoadComplete requesting Rally Points";
            _rallyPointManager->loadFromVehicle();
        } else {
            qCDebug(VehicleLog) << "_missionLoadComplete Rally Points not supported skipping";
            _rallyPointLoadComplete();
        }
    }
}

void Vehicle::_rallyPointLoadComplete(void)
{
    qCDebug(VehicleLog) << "_missionLoadComplete _initialPlanRequestComplete = true";
    if (!_initialPlanRequestComplete) {
        _initialPlanRequestComplete = true;
        emit initialPlanRequestCompleteChanged(true);
    }
}

void Vehicle::_parametersReady(bool parametersReady)
{
    // Try to set current unix time to the vehicle
    _sendQGCTimeToVehicle();
    // Send time twice, more likely to get to the vehicle on a noisy link
    _sendQGCTimeToVehicle();
    if (parametersReady) {
        _setupAutoDisarmSignalling();
        _startPlanRequest();
    }
}

void Vehicle::_sendQGCTimeToVehicle(void)
{
    mavlink_message_t       msg;
    mavlink_system_time_t   cmd;

    // Timestamp of the master clock in microseconds since UNIX epoch.
    cmd.time_unix_usec = QDateTime::currentDateTime().currentMSecsSinceEpoch()*1000;
    // Timestamp of the component clock since boot time in milliseconds (Not necessary).
    cmd.time_boot_ms = 0;
    mavlink_msg_system_time_encode_chan(_mavlink->getSystemId(),
                                        _mavlink->getComponentId(),
                                        priorityLink()->mavlinkChannel(),
                                        &msg,
                                        &cmd);

    sendMessageOnLink(priorityLink(), msg);
}

void Vehicle::disconnectInactiveVehicle(void)
{
    // Vehicle is no longer communicating with us, disconnect all links

    LinkManager* linkMgr = _toolbox->linkManager();
    for (int i=0; i<_links.count(); i++) {
        // FIXME: This linkInUse check is a hack fix for multiple vehicles on the same link.
        // The real fix requires significant restructuring which will come later.
        if (!_toolbox->multiVehicleManager()->linkInUse(_links[i], this)) {
            linkMgr->disconnectLink(_links[i]);
        }
    }
    emit linksChanged();
}

void Vehicle::_imageReady(UASInterface*)
{
    if(_uas)
    {
        QImage img = _uas->getImage();
        _toolbox->imageProvider()->setImage(&img, _id);
        _flowImageIndex++;
        emit flowImageIndexChanged();
    }
}

void Vehicle::_remoteControlRSSIChanged(uint8_t rssi)
{
    //-- 0 <= rssi <= 100 - 255 means "invalid/unknown"
    if(rssi > 100) { // Anything over 100 doesn't make sense
        if(_rcRSSI != 255) {
            _rcRSSI = 255;
            emit rcRSSIChanged(_rcRSSI);
        }
        return;
    }
    //-- Initialize it
    if(_rcRSSIstore == 255.) {
        _rcRSSIstore = (double)rssi;
    }
    // Low pass to git rid of jitter
    _rcRSSIstore = (_rcRSSIstore * 0.9f) + ((float)rssi * 0.1);
    uint8_t filteredRSSI = (uint8_t)ceil(_rcRSSIstore);
    if(_rcRSSIstore < 0.1) {
        filteredRSSI = 0;
    }
    if(_rcRSSI != filteredRSSI) {
        _rcRSSI = filteredRSSI;
        emit rcRSSIChanged(_rcRSSI);
    }
}

void Vehicle::virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust)
{
    // The following if statement prevents the virtualTabletJoystick from sending values if the standard joystick is enabled
    if ( !_joystickEnabled && !_highLatencyLink) {
        _uas->setExternalControlSetpoint(roll, pitch, yaw, thrust, 0, JoystickModeRC);
    }
}

void Vehicle::setConnectionLostEnabled(bool connectionLostEnabled)
{
    if (_connectionLostEnabled != connectionLostEnabled) {
        _connectionLostEnabled = connectionLostEnabled;
        emit connectionLostEnabledChanged(_connectionLostEnabled);
    }
}

void Vehicle::_linkActiveChanged(LinkInterface *link, bool active, int vehicleID)
{
    // only continue if the vehicle id is correct
    if (vehicleID != _id) {
        return;
    }

    emit linksPropertiesChanged();

    bool communicationLost = false;
    bool communicationRegained = false;

    if (link == _priorityLink) {
        if (active && _connectionLost) {
            // communication to priority link regained
            _connectionLost = false;
            communicationRegained = true;
            emit connectionLostChanged(false);

            if (_priorityLink->highLatency()) {
                _setMaxProtoVersion(100);
            } else {
                // Re-negotiate protocol version for the link
                sendMavCommand(MAV_COMP_ID_ALL,                         // Don't know default component id yet.
                               MAV_CMD_REQUEST_PROTOCOL_VERSION,
                               false,                                   // No error shown if fails
                               1);                                     // Request protocol version
            }
        } else if (!active && !_connectionLost) {
            _updatePriorityLink(false /* updateActive */, false /* sendCommand */);

            // check if another active link has been found
            if (link == _priorityLink) {
                _connectionLost = true;
                communicationLost = true;
                _heardFrom = false;
                _maxProtoVersion = 0;
                emit connectionLostChanged(true);

                if (_autoDisconnect) {
                    // Reset link state
                    for (int i = 0; i < _links.length(); i++) {
                        _mavlink->resetMetadataForLink(_links.at(i));
                    }

                    disconnectInactiveVehicle();
                }
            }
        }
    } else {
        qgcApp()->showMessage((tr("%1 communication to auxiliary link %2 %3")).arg(_vehicleIdSpeech()).arg(link->getName()).arg(active ? "regained" : "lost"));
        _updatePriorityLink(false /* updateActive */, true /* sendCommand */);
    }

    QString commSpeech;
    bool multiVehicle = _toolbox->multiVehicleManager()->vehicles()->count() > 1;
    if (communicationRegained) {
        commSpeech = tr("Communication regained");
        if (_links.count() > 1) {
            qgcApp()->showMessage(tr("Communication regained to vehicle %1 on %2 link %3").arg(_id).arg(_links.count() > 1 ? tr("priority") : tr("auxiliary")).arg(link->getName()));
        } else if (multiVehicle) {
            qgcApp()->showMessage(tr("Communication regained to vehicle %1").arg(_id));
        }
    }
    if (communicationLost) {
        commSpeech = tr("Communication lost");
        if (_links.count() > 1) {
            qgcApp()->showMessage(tr("Communication lost to vehicle %1 on %2 link %3").arg(_id).arg(_links.count() > 1 ? tr("priority") : tr("auxiliary")).arg(link->getName()));
        } else if (multiVehicle) {
            qgcApp()->showMessage(tr("Communication lost to vehicle %1").arg(_id));
        }
    }
    if (multiVehicle && (communicationLost || communicationRegained)) {
        commSpeech.append(tr(" to vehicle %1").arg(_id));
    }
    if (!commSpeech.isEmpty()) {
        _say(commSpeech);
    }
}

void Vehicle::_say(const QString& text)
{
    _toolbox->audioOutput()->say(text.toLower());
}

bool Vehicle::fixedWing(void) const
{
    return QGCMAVLink::isFixedWing(vehicleType());
}

bool Vehicle::rover(void) const
{
    return QGCMAVLink::isRover(vehicleType());
}

bool Vehicle::sub(void) const
{
    return QGCMAVLink::isSub(vehicleType());
}

bool Vehicle::multiRotor(void) const
{
    return QGCMAVLink::isMultiRotor(vehicleType());
}

bool Vehicle::vtol(void) const
{
    return _firmwarePlugin->isVtol(this);
}

bool Vehicle::supportsThrottleModeCenterZero(void) const
{
    return _firmwarePlugin->supportsThrottleModeCenterZero();
}

bool Vehicle::supportsNegativeThrust(void) const
{
    return _firmwarePlugin->supportsNegativeThrust();
}

bool Vehicle::supportsRadio(void) const
{
    return _firmwarePlugin->supportsRadio();
}

bool Vehicle::supportsJSButton(void) const
{
    return _firmwarePlugin->supportsJSButton();
}

bool Vehicle::supportsMotorInterference(void) const
{
    return _firmwarePlugin->supportsMotorInterference();
}

bool Vehicle::supportsTerrainFrame(void) const
{
    return _firmwarePlugin->supportsTerrainFrame();
}

QString Vehicle::vehicleTypeName() const {
    static QMap<int, QString> typeNames = {
        { MAV_TYPE_GENERIC,         tr("Generic micro air vehicle" )},
        { MAV_TYPE_FIXED_WING,      tr("Fixed wing aircraft")},
        { MAV_TYPE_QUADROTOR,       tr("Quadrotor")},
        { MAV_TYPE_COAXIAL,         tr("Coaxial helicopter")},
        { MAV_TYPE_HELICOPTER,      tr("Normal helicopter with tail rotor.")},
        { MAV_TYPE_ANTENNA_TRACKER, tr("Ground installation")},
        { MAV_TYPE_GCS,             tr("Operator control unit / ground control station")},
        { MAV_TYPE_AIRSHIP,         tr("Airship, controlled")},
        { MAV_TYPE_FREE_BALLOON,    tr("Free balloon, uncontrolled")},
        { MAV_TYPE_ROCKET,          tr("Rocket")},
        { MAV_TYPE_GROUND_ROVER,    tr("Ground rover")},
        { MAV_TYPE_SURFACE_BOAT,    tr("Surface vessel, boat, ship")},
        { MAV_TYPE_SUBMARINE,       tr("Submarine")},
        { MAV_TYPE_HEXAROTOR,       tr("Hexarotor")},
        { MAV_TYPE_OCTOROTOR,       tr("Octorotor")},
        { MAV_TYPE_TRICOPTER,       tr("Octorotor")},
        { MAV_TYPE_FLAPPING_WING,   tr("Flapping wing")},
        { MAV_TYPE_KITE,            tr("Flapping wing")},
        { MAV_TYPE_ONBOARD_CONTROLLER, tr("Onboard companion controller")},
        { MAV_TYPE_VTOL_DUOROTOR,   tr("Two-rotor VTOL using control surfaces in vertical operation in addition. Tailsitter")},
        { MAV_TYPE_VTOL_QUADROTOR,  tr("Quad-rotor VTOL using a V-shaped quad config in vertical operation. Tailsitter")},
        { MAV_TYPE_VTOL_TILTROTOR,  tr("Tiltrotor VTOL")},
        { MAV_TYPE_VTOL_RESERVED2,  tr("VTOL reserved 2")},
        { MAV_TYPE_VTOL_RESERVED3,  tr("VTOL reserved 3")},
        { MAV_TYPE_VTOL_RESERVED4,  tr("VTOL reserved 4")},
        { MAV_TYPE_VTOL_RESERVED5,  tr("VTOL reserved 5")},
        { MAV_TYPE_GIMBAL,          tr("Onboard gimbal")},
        { MAV_TYPE_ADSB,            tr("Onboard ADSB peripheral")},
    };
    return typeNames[_vehicleType];
}

/// Returns the string to speak to identify the vehicle
QString Vehicle::_vehicleIdSpeech(void)
{
    if (_toolbox->multiVehicleManager()->vehicles()->count() > 1) {
        return QString(tr("vehicle %1")).arg(id());
    } else {
        return QString();
    }
}

void Vehicle::_handleFlightModeChanged(const QString& flightMode)
{
    _say(QString(tr("%1 %2 flight mode")).arg(_vehicleIdSpeech()).arg(flightMode));
    emit guidedModeChanged(_firmwarePlugin->isGuidedMode(this));
}

void Vehicle::_announceArmedChanged(bool armed)
{
    _say(QString("%1 %2").arg(_vehicleIdSpeech()).arg(armed ? QString(tr("armed")) : QString(tr("disarmed"))));
}

void Vehicle::_setFlying(bool flying)
{
    if (_flying != flying) {
        _flying = flying;
        emit flyingChanged(flying);
    }
}

void Vehicle::_setLanding(bool landing)
{
    if (armed() && _landing != landing) {
        _landing = landing;
        emit landingChanged(landing);
    }
}

bool Vehicle::guidedModeSupported(void) const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::GuidedModeCapability);
}

bool Vehicle::pauseVehicleSupported(void) const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::PauseVehicleCapability);
}

bool Vehicle::orbitModeSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::OrbitModeCapability);
}

bool Vehicle::takeoffVehicleSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::TakeoffVehicleCapability);
}

void Vehicle::guidedModeRTL(void)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeRTL(this);
}

void Vehicle::guidedModeLand(void)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeLand(this);
}

void Vehicle::guidedModeTakeoff(double altitudeRelative)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeTakeoff(this, altitudeRelative);
}

double Vehicle::minimumTakeoffAltitude(void)
{
    return _firmwarePlugin->minimumTakeoffAltitude(this);
}

void Vehicle::startMission(void)
{
    _firmwarePlugin->startMission(this);
}

void Vehicle::guidedModeGotoLocation(const QGeoCoordinate& gotoCoord)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    if (!coordinate().isValid()) {
        return;
    }
    double maxDistance = 1000.0;
    if (coordinate().distanceTo(gotoCoord) > maxDistance) {
        qgcApp()->showMessage(QString("New location is too far. Must be less than %1 %2").arg(qRound(FactMetaData::metersToAppSettingsDistanceUnits(maxDistance).toDouble())).arg(FactMetaData::appSettingsDistanceUnitsString()));
        return;
    }
    _firmwarePlugin->guidedModeGotoLocation(this, gotoCoord);
}

void Vehicle::guidedModeChangeAltitude(double altitudeChange)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeAltitude(this, altitudeChange);
}

void Vehicle::guidedModeOrbit(const QGeoCoordinate& centerCoord, double radius, double amslAltitude)
{
    if (!orbitModeSupported()) {
        qgcApp()->showMessage(QStringLiteral("Orbit mode not supported by Vehicle."));
        return;
    }

    if (capabilityBits() && MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        sendMavCommandInt(defaultComponentId(),
                          MAV_CMD_DO_ORBIT,
                          MAV_FRAME_GLOBAL,
                          true,            // show error if fails
                          radius,
                          qQNaN(),         // Use default velocity
                          0,               // Vehicle points to center
                          qQNaN(),         // reserved
                          centerCoord.latitude(), centerCoord.longitude(), amslAltitude);
    } else {
        sendMavCommand(defaultComponentId(),
                       MAV_CMD_DO_ORBIT,
                       true,            // show error if fails
                       radius,
                       qQNaN(),         // Use default velocity
                       0,               // Vehicle points to center
                       qQNaN(),         // reserved
                       centerCoord.latitude(), centerCoord.longitude(), amslAltitude);
    }
}

void Vehicle::pauseVehicle(void)
{
    if (!pauseVehicleSupported()) {
        qgcApp()->showMessage(QStringLiteral("Pause not supported by vehicle."));
        return;
    }
    _firmwarePlugin->pauseVehicle(this);
}

void Vehicle::abortLanding(double climbOutAltitude)
{
    sendMavCommand(defaultComponentId(),
                   MAV_CMD_DO_GO_AROUND,
                   true,        // show error if fails
                   climbOutAltitude);
}

bool Vehicle::guidedMode(void) const
{
    return _firmwarePlugin->isGuidedMode(this);
}

void Vehicle::setGuidedMode(bool guidedMode)
{
    return _firmwarePlugin->setGuidedMode(this, guidedMode);
}

void Vehicle::emergencyStop(void)
{
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_COMPONENT_ARM_DISARM,
                   true,        // show error if fails
                   0.0f,
                   21196.0f);  // Magic number for emergency stop
}

void Vehicle::setCurrentMissionSequence(int seq)
{
    if (!_firmwarePlugin->sendHomePositionToVehicle()) {
        seq--;
    }
    mavlink_message_t msg;
    mavlink_msg_mission_set_current_pack_chan(_mavlink->getSystemId(),
                                              _mavlink->getComponentId(),
                                              priorityLink()->mavlinkChannel(),
                                              &msg,
                                              id(),
                                              _compID,
                                              seq);
    sendMessageOnLink(priorityLink(), msg);
}

void Vehicle::sendMavCommand(int component, MAV_CMD command, bool showError, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    MavCommandQueueEntry_t entry;

    entry.commandInt = false;
    entry.component = component;
    entry.command = command;
    entry.showError = showError;
    entry.rgParam[0] = param1;
    entry.rgParam[1] = param2;
    entry.rgParam[2] = param3;
    entry.rgParam[3] = param4;
    entry.rgParam[4] = param5;
    entry.rgParam[5] = param6;
    entry.rgParam[6] = param7;

    _mavCommandQueue.append(entry);

    if (_mavCommandQueue.count() == 1) {
        _mavCommandRetryCount = 0;
        _sendMavCommandAgain();
    }
}

void Vehicle::sendMavCommandInt(int component, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    MavCommandQueueEntry_t entry;

    entry.commandInt = true;
    entry.component = component;
    entry.command = command;
    entry.frame = frame;
    entry.showError = showError;
    entry.rgParam[0] = param1;
    entry.rgParam[1] = param2;
    entry.rgParam[2] = param3;
    entry.rgParam[3] = param4;
    entry.rgParam[4] = param5;
    entry.rgParam[5] = param6;
    entry.rgParam[6] = param7;

    _mavCommandQueue.append(entry);

    if (_mavCommandQueue.count() == 1) {
        _mavCommandRetryCount = 0;
        _sendMavCommandAgain();
    }
}

void Vehicle::_sendMavCommandAgain(void)
{
    if(!_mavCommandQueue.size()) {
        qWarning() << "Command resend with no commands in queue";
        _mavCommandAckTimer.stop();
        return;
    }

    MavCommandQueueEntry_t& queuedCommand = _mavCommandQueue[0];

    if (_mavCommandRetryCount++ > _mavCommandMaxRetryCount) {
        if (queuedCommand.command == MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES) {
            // We aren't going to get a response back for capabilities, so stop waiting for it before we ask for mission items
            qCDebug(VehicleLog) << "Vehicle failed to responded to MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES. Setting no capabilities. Starting Plan request.";
            _setCapabilities(0);
            _startPlanRequest();
        }

        if (queuedCommand.command == MAV_CMD_REQUEST_PROTOCOL_VERSION) {
            // We aren't going to get a response back for the protocol version, so assume v1 is all we can do.
            // If the max protocol version is uninitialized, fall back to v1.
            qCDebug(VehicleLog) << "Vehicle failed to responded to MAV_CMD_REQUEST_PROTOCOL_VERSION. Starting Plan request.";
            if (_maxProtoVersion == 0) {
                qCDebug(VehicleLog) << "Setting _maxProtoVersion to 100 since not yet set.";
                _setMaxProtoVersion(100);
            } else {
                qCDebug(VehicleLog) << "Leaving _maxProtoVersion at current value" << _maxProtoVersion;
            }
        }

        emit mavCommandResult(_id, queuedCommand.component, queuedCommand.command, MAV_RESULT_FAILED, true /* noResponsefromVehicle */);
        if (queuedCommand.showError) {
            qgcApp()->showMessage(tr("Vehicle did not respond to command: %1").arg(_toolbox->missionCommandTree()->friendlyName(queuedCommand.command)));
        }
        _mavCommandQueue.removeFirst();
        _sendNextQueuedMavCommand();
        return;
    }

    if (_mavCommandRetryCount > 1) {
        // We always let AUTOPILOT_CAPABILITIES go through multiple times even if we don't get acks. This is because
        // we really need to get capabilities and version info back over a lossy link.
        if (queuedCommand.command != MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES) {
            if (px4Firmware()) {
                // Older PX4 firmwares are inconsistent with repect to sending back an Ack from a COMMAND_LONG, hence we can't support retry logic for it.
                if (_firmwareMajorVersion != versionNotSetValue) {
                    // If no version set assume lastest master dev build, so acks are suppored
                    if (_firmwareMajorVersion <= 1 && _firmwareMinorVersion <= 5 && _firmwarePatchVersion <= 3) {
                        // Acks not supported in this version
                        return;
                    }
                }
            } else {
                if (queuedCommand.command == MAV_CMD_START_RX_PAIR) {
                    // The implementation of this command comes from the IO layer and is shared across stacks. So for other firmwares
                    // we aren't really sure whether they are correct or not.
                    return;
                }
            }
        }
        qCDebug(VehicleLog) << "Vehicle::_sendMavCommandAgain retrying command:_mavCommandRetryCount" << queuedCommand.command << _mavCommandRetryCount;
    }

    _mavCommandAckTimer.start();

    mavlink_message_t       msg;
    if (queuedCommand.commandInt) {
        mavlink_command_int_t  cmd;

        memset(&cmd, 0, sizeof(cmd));
        cmd.target_system =     _id;
        cmd.target_component =  queuedCommand.component;
        cmd.command =           queuedCommand.command;
        cmd.frame =             queuedCommand.frame;
        cmd.param1 =            queuedCommand.rgParam[0];
        cmd.param2 =            queuedCommand.rgParam[1];
        cmd.param3 =            queuedCommand.rgParam[2];
        cmd.param4 =            queuedCommand.rgParam[3];
        cmd.x =                 queuedCommand.rgParam[4] * qPow(10.0, 7.0);
        cmd.y =                 queuedCommand.rgParam[5] * qPow(10.0, 7.0);
        cmd.z =                 queuedCommand.rgParam[6];
        mavlink_msg_command_int_encode_chan(_mavlink->getSystemId(),
                                            _mavlink->getComponentId(),
                                            priorityLink()->mavlinkChannel(),
                                            &msg,
                                            &cmd);
    } else {
        mavlink_command_long_t  cmd;

        memset(&cmd, 0, sizeof(cmd));
        cmd.target_system =     _id;
        cmd.target_component =  queuedCommand.component;
        cmd.command =           queuedCommand.command;
        cmd.confirmation =      0;
        cmd.param1 =            queuedCommand.rgParam[0];
        cmd.param2 =            queuedCommand.rgParam[1];
        cmd.param3 =            queuedCommand.rgParam[2];
        cmd.param4 =            queuedCommand.rgParam[3];
        cmd.param5 =            queuedCommand.rgParam[4];
        cmd.param6 =            queuedCommand.rgParam[5];
        cmd.param7 =            queuedCommand.rgParam[6];
        mavlink_msg_command_long_encode_chan(_mavlink->getSystemId(),
                                             _mavlink->getComponentId(),
                                             priorityLink()->mavlinkChannel(),
                                             &msg,
                                             &cmd);
    }

    sendMessageOnLink(priorityLink(), msg);
}

void Vehicle::_sendNextQueuedMavCommand(void)
{
    if (_mavCommandQueue.count()) {
        _mavCommandRetryCount = 0;
        _sendMavCommandAgain();
    }
}


void Vehicle::_handleCommandAck(mavlink_message_t& message)
{
    bool showError = false;

    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&message, &ack);

    if (ack.command == MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES && ack.result != MAV_RESULT_ACCEPTED) {
        // We aren't going to get a response back for capabilities, so stop waiting for it before we ask for mission items
        qCDebug(VehicleLog) << QStringLiteral("Vehicle responded to MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES with error(%1). Setting no capabilities. Starting Plan request.").arg(ack.result);
        _setCapabilities(0);
    }

    if (ack.command == MAV_CMD_REQUEST_PROTOCOL_VERSION && ack.result != MAV_RESULT_ACCEPTED) {
        // The autopilot does not understand the request and consequently is likely handling only
        // MAVLink 1
        qCDebug(VehicleLog) << QStringLiteral("Vehicle responded to MAV_CMD_REQUEST_PROTOCOL_VERSION with error(%1).").arg(ack.result);
        if (_maxProtoVersion == 0) {
            qCDebug(VehicleLog) << "Setting _maxProtoVersion to 100 since not yet set.";
            _setMaxProtoVersion(100);
        } else {
            qCDebug(VehicleLog) << "Leaving _maxProtoVersion at current value" << _maxProtoVersion;
        }
        // FIXME: Is this missing here. I believe it is a bug. Debug to verify. May need to go into Stable.
        //_startPlanRequest();
    }

    if (_mavCommandQueue.count() && ack.command == _mavCommandQueue[0].command) {
        _mavCommandAckTimer.stop();
        showError = _mavCommandQueue[0].showError;
        _mavCommandQueue.removeFirst();
    }

    emit mavCommandResult(_id, message.compid, ack.command, ack.result, false /* noResponsefromVehicle */);

    if (showError) {
        QString commandName = _toolbox->missionCommandTree()->friendlyName((MAV_CMD)ack.command);

        switch (ack.result) {
        case MAV_RESULT_TEMPORARILY_REJECTED:
            qgcApp()->showMessage(tr("%1 command temporarily rejected").arg(commandName));
            break;
        case MAV_RESULT_DENIED:
            qgcApp()->showMessage(tr("%1 command denied").arg(commandName));
            break;
        case MAV_RESULT_UNSUPPORTED:
            qgcApp()->showMessage(tr("%1 command not supported").arg(commandName));
            break;
        case MAV_RESULT_FAILED:
            qgcApp()->showMessage(tr("%1 command failed").arg(commandName));
            break;
        default:
            // Do nothing
            break;
        }
    }

    _sendNextQueuedMavCommand();
}

void Vehicle::setPrearmError(const QString& prearmError)
{
    _prearmError = prearmError;
    emit prearmErrorChanged(_prearmError);
    if (!_prearmError.isEmpty()) {
        _prearmErrorTimer.start();
    }
}

void Vehicle::_prearmErrorTimeout(void)
{
    setPrearmError(QString());
}

void Vehicle::setFirmwareVersion(int majorVersion, int minorVersion, int patchVersion, FIRMWARE_VERSION_TYPE versionType)
{
    _firmwareMajorVersion = majorVersion;
    _firmwareMinorVersion = minorVersion;
    _firmwarePatchVersion = patchVersion;
    _firmwareVersionType = versionType;
    emit firmwareVersionChanged();
}

void Vehicle::setFirmwareCustomVersion(int majorVersion, int minorVersion, int patchVersion)
{
    _firmwareCustomMajorVersion = majorVersion;
    _firmwareCustomMinorVersion = minorVersion;
    _firmwareCustomPatchVersion = patchVersion;
    emit firmwareCustomVersionChanged();
}

QString Vehicle::firmwareVersionTypeString(void) const
{
    switch (_firmwareVersionType) {
    case FIRMWARE_VERSION_TYPE_DEV:
        return QStringLiteral("dev");
    case FIRMWARE_VERSION_TYPE_ALPHA:
        return QStringLiteral("alpha");
    case FIRMWARE_VERSION_TYPE_BETA:
        return QStringLiteral("beta");
    case FIRMWARE_VERSION_TYPE_RC:
        return QStringLiteral("rc");
    case FIRMWARE_VERSION_TYPE_OFFICIAL:
    default:
        return QStringLiteral("");
    }
}

void Vehicle::rebootVehicle()
{
    _autoDisconnect = true;
    sendMavCommand(_defaultComponentId, MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, true, 1.0f);
}

void Vehicle::setSoloFirmware(bool soloFirmware)
{
    if (soloFirmware != _soloFirmware) {
        _soloFirmware = soloFirmware;
        emit soloFirmwareChanged(soloFirmware);
    }
}

#if 0
// Temporarily removed, waiting for new command implementation
void Vehicle::motorTest(int motor, int percent, int timeoutSecs)
{
    doCommandLongUnverified(_defaultComponentId, MAV_CMD_DO_MOTOR_TEST, motor, MOTOR_TEST_THROTTLE_PERCENT, percent, timeoutSecs);
}
#endif

QString Vehicle::brandImageIndoor(void) const
{
    return _firmwarePlugin->brandImageIndoor(this);
}

QString Vehicle::brandImageOutdoor(void) const
{
    return _firmwarePlugin->brandImageOutdoor(this);
}

QStringList Vehicle::unhealthySensors(void) const
{
    QStringList sensorList;

    struct sensorInfo_s {
        uint32_t    bit;
        const char* sensorName;
    };

    static const sensorInfo_s rgSensorInfo[] = {
        { MAV_SYS_STATUS_SENSOR_3D_GYRO,                "Gyro" },
        { MAV_SYS_STATUS_SENSOR_3D_ACCEL,               "Accelerometer" },
        { MAV_SYS_STATUS_SENSOR_3D_MAG,                 "Magnetometer" },
        { MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE,      "Absolute pressure" },
        { MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE,  "Differential pressure" },
        { MAV_SYS_STATUS_SENSOR_GPS,                    "GPS" },
        { MAV_SYS_STATUS_SENSOR_OPTICAL_FLOW,           "Optical flow" },
        { MAV_SYS_STATUS_SENSOR_VISION_POSITION,        "Computer vision position" },
        { MAV_SYS_STATUS_SENSOR_LASER_POSITION,         "Laser based position" },
        { MAV_SYS_STATUS_SENSOR_EXTERNAL_GROUND_TRUTH,  "External ground truth" },
        { MAV_SYS_STATUS_SENSOR_ANGULAR_RATE_CONTROL,   "Angular rate control" },
        { MAV_SYS_STATUS_SENSOR_ATTITUDE_STABILIZATION, "Attitude stabilization" },
        { MAV_SYS_STATUS_SENSOR_YAW_POSITION,           "Yaw position" },
        { MAV_SYS_STATUS_SENSOR_Z_ALTITUDE_CONTROL,     "Z/altitude control" },
        { MAV_SYS_STATUS_SENSOR_XY_POSITION_CONTROL,    "X/Y position control" },
        { MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS,          "Motor outputs / control" },
        { MAV_SYS_STATUS_SENSOR_RC_RECEIVER,            "RC receiver" },
        { MAV_SYS_STATUS_SENSOR_3D_GYRO2,               "Gyro 2" },
        { MAV_SYS_STATUS_SENSOR_3D_ACCEL2,              "Accelerometer 2" },
        { MAV_SYS_STATUS_SENSOR_3D_MAG2,                "Magnetometer 2" },
        { MAV_SYS_STATUS_GEOFENCE,                      "GeoFence" },
        { MAV_SYS_STATUS_AHRS,                          "AHRS" },
        { MAV_SYS_STATUS_TERRAIN,                       "Terrain" },
        { MAV_SYS_STATUS_REVERSE_MOTOR,                 "Motors reversed" },
        { MAV_SYS_STATUS_LOGGING,                       "Logging" },
        { MAV_SYS_STATUS_SENSOR_BATTERY,                "Battery" },
    };

    for (size_t i=0; i<sizeof(rgSensorInfo)/sizeof(sensorInfo_s); i++) {
        const sensorInfo_s* pSensorInfo = &rgSensorInfo[i];
        if ((_onboardControlSensorsEnabled & pSensorInfo->bit) && !(_onboardControlSensorsHealth & pSensorInfo->bit)) {
            sensorList << pSensorInfo->sensorName;
        }
    }

    return sensorList;
}

void Vehicle::setOfflineEditingDefaultComponentId(int defaultComponentId)
{
    if (_offlineEditingVehicle) {
        _defaultComponentId = defaultComponentId;
    } else {
        qWarning() << "Call to Vehicle::setOfflineEditingDefaultComponentId on vehicle which is not offline";
    }
}

void Vehicle::triggerCamera(void)
{
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_DO_DIGICAM_CONTROL,
                   true,                            // show errors
                   0.0, 0.0, 0.0, 0.0,              // param 1-4 unused
                   1.0,                             // trigger camera
                   0.0,                             // param 6 unused
                   1.0);                            // test shot flag
}

void Vehicle::setVtolInFwdFlight(bool vtolInFwdFlight)
{
    if (_vtolInFwdFlight != vtolInFwdFlight) {
        sendMavCommand(_defaultComponentId,
                       MAV_CMD_DO_VTOL_TRANSITION,
                       true,                                                    // show errors
                       vtolInFwdFlight ? MAV_VTOL_STATE_FW : MAV_VTOL_STATE_MC, // transition state
                       0, 0, 0, 0, 0, 0);                                       // param 2-7 unused
    }
}

const char* VehicleGPSFactGroup::_latFactName =                 "lat";
const char* VehicleGPSFactGroup::_lonFactName =                 "lon";
const char* VehicleGPSFactGroup::_hdopFactName =                "hdop";
const char* VehicleGPSFactGroup::_vdopFactName =                "vdop";
const char* VehicleGPSFactGroup::_courseOverGroundFactName =    "courseOverGround";
const char* VehicleGPSFactGroup::_countFactName =               "count";
const char* VehicleGPSFactGroup::_lockFactName =                "lock";

VehicleGPSFactGroup::VehicleGPSFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/GPSFact.json", parent)
    , _latFact              (0, _latFactName,               FactMetaData::valueTypeDouble)
    , _lonFact              (0, _lonFactName,               FactMetaData::valueTypeDouble)
    , _hdopFact             (0, _hdopFactName,              FactMetaData::valueTypeDouble)
    , _vdopFact             (0, _vdopFactName,              FactMetaData::valueTypeDouble)
    , _courseOverGroundFact (0, _courseOverGroundFactName,  FactMetaData::valueTypeDouble)
    , _countFact            (0, _countFactName,             FactMetaData::valueTypeInt32)
    , _lockFact             (0, _lockFactName,              FactMetaData::valueTypeInt32)
{
    _addFact(&_latFact,                 _latFactName);
    _addFact(&_lonFact,                 _lonFactName);
    _addFact(&_hdopFact,                _hdopFactName);
    _addFact(&_vdopFact,                _vdopFactName);
    _addFact(&_courseOverGroundFact,    _courseOverGroundFactName);
    _addFact(&_lockFact,                _lockFactName);
    _addFact(&_countFact,               _countFactName);

    _latFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _lonFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _hdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _vdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _courseOverGroundFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

void Vehicle::startMavlinkLog()
{
    sendMavCommand(_defaultComponentId, MAV_CMD_LOGGING_START, false /* showError */);
}

void Vehicle::stopMavlinkLog()
{
    sendMavCommand(_defaultComponentId, MAV_CMD_LOGGING_STOP, false /* showError */);
}

void Vehicle::_ackMavlinkLogData(uint16_t sequence)
{
    mavlink_message_t msg;
    mavlink_logging_ack_t ack;
    memset(&ack, 0, sizeof(ack));
    ack.sequence = sequence;
    ack.target_component = _defaultComponentId;
    ack.target_system = id();
    mavlink_msg_logging_ack_encode_chan(
                _mavlink->getSystemId(),
                _mavlink->getComponentId(),
                priorityLink()->mavlinkChannel(),
                &msg,
                &ack);
    sendMessageOnLink(priorityLink(), msg);
}

void Vehicle::_handleMavlinkLoggingData(mavlink_message_t& message)
{
    mavlink_logging_data_t log;
    mavlink_msg_logging_data_decode(&message, &log);
    emit mavlinkLogData(this, log.target_system, log.target_component, log.sequence,
                        log.first_message_offset, QByteArray((const char*)log.data, log.length), false);
}

void Vehicle::_handleMavlinkLoggingDataAcked(mavlink_message_t& message)
{
    mavlink_logging_data_acked_t log;
    mavlink_msg_logging_data_acked_decode(&message, &log);
    _ackMavlinkLogData(log.sequence);
    emit mavlinkLogData(this, log.target_system, log.target_component, log.sequence,
                        log.first_message_offset, QByteArray((const char*)log.data, log.length), true);
}

void Vehicle::setFirmwarePluginInstanceData(QObject* firmwarePluginInstanceData)
{
    firmwarePluginInstanceData->setParent(this);
    _firmwarePluginInstanceData = firmwarePluginInstanceData;
}

QString Vehicle::missionFlightMode(void) const
{
    return _firmwarePlugin->missionFlightMode();
}

QString Vehicle::pauseFlightMode(void) const
{
    return _firmwarePlugin->pauseFlightMode();
}

QString Vehicle::rtlFlightMode(void) const
{
    return _firmwarePlugin->rtlFlightMode();
}

QString Vehicle::landFlightMode(void) const
{
    return _firmwarePlugin->landFlightMode();
}

QString Vehicle::takeControlFlightMode(void) const
{
    return _firmwarePlugin->takeControlFlightMode();
}

QString Vehicle::vehicleImageOpaque() const
{
    if(_firmwarePlugin)
        return _firmwarePlugin->vehicleImageOpaque(this);
    else
        return QString();
}

QString Vehicle::vehicleImageOutline() const
{
    if(_firmwarePlugin)
        return _firmwarePlugin->vehicleImageOutline(this);
    else
        return QString();
}

QString Vehicle::vehicleImageCompass() const
{
    if(_firmwarePlugin)
        return _firmwarePlugin->vehicleImageCompass(this);
    else
        return QString();
}

const QVariantList& Vehicle::toolBarIndicators()
{
    if(_firmwarePlugin) {
        return _firmwarePlugin->toolBarIndicators(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

const QVariantList& Vehicle::staticCameraList(void) const
{
    if (_firmwarePlugin) {
        return _firmwarePlugin->cameraList(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

bool Vehicle::vehicleYawsToNextWaypointInMission(void) const
{
    return _firmwarePlugin->vehicleYawsToNextWaypointInMission(this);
}

void Vehicle::_setupAutoDisarmSignalling(void)
{
    QString param = _firmwarePlugin->autoDisarmParameter(this);

    if (!param.isEmpty() && _parameterManager->parameterExists(FactSystem::defaultComponentId, param)) {
        Fact* fact = _parameterManager->getParameter(FactSystem::defaultComponentId,param);
        connect(fact, &Fact::rawValueChanged, this, &Vehicle::autoDisarmChanged);
        emit autoDisarmChanged();
    }
}

bool Vehicle::autoDisarm(void)
{
    QString param = _firmwarePlugin->autoDisarmParameter(this);

    if (!param.isEmpty() && _parameterManager->parameterExists(FactSystem::defaultComponentId, param)) {
        Fact* fact = _parameterManager->getParameter(FactSystem::defaultComponentId,param);
        return fact->rawValue().toDouble() > 0;
    }

    return false;
}

void Vehicle::_handleADSBVehicle(const mavlink_message_t& message)
{
    mavlink_adsb_vehicle_t adsbVehicle;
    static const int maxTimeSinceLastSeen = 15;

    mavlink_msg_adsb_vehicle_decode(&message, &adsbVehicle);
    if (adsbVehicle.flags | ADSB_FLAGS_VALID_COORDS) {
        if (_adsbICAOMap.contains(adsbVehicle.ICAO_address)) {
            if (adsbVehicle.tslc > maxTimeSinceLastSeen) {
                ADSBVehicle* vehicle = _adsbICAOMap[adsbVehicle.ICAO_address];
                _adsbVehicles.removeOne(vehicle);
                _adsbICAOMap.remove(adsbVehicle.ICAO_address);
                vehicle->deleteLater();
            } else {
                _adsbICAOMap[adsbVehicle.ICAO_address]->update(adsbVehicle);
            }
        } else if (adsbVehicle.tslc <= maxTimeSinceLastSeen) {
            ADSBVehicle* vehicle = new ADSBVehicle(adsbVehicle, this);
            _adsbICAOMap[adsbVehicle.ICAO_address] = vehicle;
            _adsbVehicles.append(vehicle);
        }
    }
}

void Vehicle::_updateDistanceToHome(void)
{
    if (coordinate().isValid() && homePosition().isValid()) {
        _distanceToHomeFact.setRawValue(coordinate().distanceTo(homePosition()));
    } else {
        _distanceToHomeFact.setRawValue(qQNaN());
    }
}

void Vehicle::_updateHobbsMeter(void)
{
    _hobbsFact.setRawValue(hobbsMeter());
}

void Vehicle::forceInitialPlanRequestComplete(void)
{
    _initialPlanRequestComplete = true;
    emit initialPlanRequestCompleteChanged(true);
}

void Vehicle::sendPlan(QString planFile)
{
    PlanMasterController::sendPlanToVehicle(this, planFile);
}

QString Vehicle::hobbsMeter()
{
    static const char* HOOBS_HI = "LND_FLIGHT_T_HI";
    static const char* HOOBS_LO = "LND_FLIGHT_T_LO";
    //-- TODO: Does this exist on non PX4?
    if (_parameterManager->parameterExists(FactSystem::defaultComponentId, HOOBS_HI) &&
            _parameterManager->parameterExists(FactSystem::defaultComponentId, HOOBS_LO)) {
        Fact* factHi = _parameterManager->getParameter(FactSystem::defaultComponentId, HOOBS_HI);
        Fact* factLo = _parameterManager->getParameter(FactSystem::defaultComponentId, HOOBS_LO);
        uint64_t hobbsTimeSeconds = ((uint64_t)factHi->rawValue().toUInt() << 32 | (uint64_t)factLo->rawValue().toUInt()) / 1000000;
        int hours   = hobbsTimeSeconds / 3600;
        int minutes = (hobbsTimeSeconds % 3600) / 60;
        int seconds = hobbsTimeSeconds % 60;
        QString timeStr;
        timeStr.sprintf("%04d:%02d:%02d", hours, minutes, seconds);
        qCDebug(VehicleLog) << "Hobbs Meter:" << timeStr << "(" << factHi->rawValue().toUInt() << factLo->rawValue().toUInt() << ")";
        return timeStr;
    }
    return QString("0000:00:00");
}

void Vehicle::_vehicleParamLoaded(bool ready)
{
    //-- TODO: This seems silly but can you think of a better
    //   way to update this?
    if(ready) {
        emit hobbsMeterChanged();
    }
}

void Vehicle::_updateHighLatencyLink(bool sendCommand)
{
    if (!_priorityLink) {
        return;
    }

    if (_priorityLink->highLatency() != _highLatencyLink) {
        _highLatencyLink = _priorityLink->highLatency();
        _mavCommandAckTimer.setInterval(_highLatencyLink ? _mavCommandAckTimeoutMSecsHighLatency : _mavCommandAckTimeoutMSecs);
        emit highLatencyLinkChanged(_highLatencyLink);

        if (sendCommand) {
            sendMavCommand(defaultComponentId(),
                           MAV_CMD_CONTROL_HIGH_LATENCY,
                           true,
                           _highLatencyLink ? 1.0f : 0.0f); // request start/stop transmitting over high latency telemetry
        }
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const char* VehicleBatteryFactGroup::_voltageFactName =                     "voltage";
const char* VehicleBatteryFactGroup::_percentRemainingFactName =            "percentRemaining";
const char* VehicleBatteryFactGroup::_mahConsumedFactName =                 "mahConsumed";
const char* VehicleBatteryFactGroup::_currentFactName =                     "current";
const char* VehicleBatteryFactGroup::_temperatureFactName =                 "temperature";
const char* VehicleBatteryFactGroup::_cellCountFactName =                   "cellCount";
const char* VehicleBatteryFactGroup::_instantPowerFactName =                "instantPower";
const char* VehicleBatteryFactGroup::_timeRemainingFactName =               "timeRemaining";
const char* VehicleBatteryFactGroup::_chargeStateFactName =                 "chargeState";

const char* VehicleBatteryFactGroup::_settingsGroup =                       "Vehicle.battery";

const double VehicleBatteryFactGroup::_voltageUnavailable =           -1.0;
const int    VehicleBatteryFactGroup::_percentRemainingUnavailable =  -1;
const int    VehicleBatteryFactGroup::_mahConsumedUnavailable =       -1;
const int    VehicleBatteryFactGroup::_currentUnavailable =           -1;
const double VehicleBatteryFactGroup::_temperatureUnavailable =       -1.0;
const int    VehicleBatteryFactGroup::_cellCountUnavailable =         -1.0;
const double VehicleBatteryFactGroup::_instantPowerUnavailable =      -1.0;

VehicleBatteryFactGroup::VehicleBatteryFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/BatteryFact.json", parent)
    , _voltageFact                  (0, _voltageFactName,                   FactMetaData::valueTypeDouble)
    , _percentRemainingFact         (0, _percentRemainingFactName,          FactMetaData::valueTypeInt32)
    , _mahConsumedFact              (0, _mahConsumedFactName,               FactMetaData::valueTypeInt32)
    , _currentFact                  (0, _currentFactName,                   FactMetaData::valueTypeFloat)
    , _temperatureFact              (0, _temperatureFactName,               FactMetaData::valueTypeDouble)
    , _cellCountFact                (0, _cellCountFactName,                 FactMetaData::valueTypeInt32)
    , _instantPowerFact             (0, _instantPowerFactName,              FactMetaData::valueTypeFloat)
    , _timeRemainingFact            (0, _timeRemainingFactName,             FactMetaData::valueTypeInt32)
    , _chargeStateFact              (0, _chargeStateFactName,               FactMetaData::valueTypeUint8)
{
    _addFact(&_voltageFact,                 _voltageFactName);
    _addFact(&_percentRemainingFact,        _percentRemainingFactName);
    _addFact(&_mahConsumedFact,             _mahConsumedFactName);
    _addFact(&_currentFact,                 _currentFactName);
    _addFact(&_temperatureFact,             _temperatureFactName);
    _addFact(&_cellCountFact,               _cellCountFactName);
    _addFact(&_instantPowerFact,            _instantPowerFactName);
    _addFact(&_timeRemainingFact,           _timeRemainingFactName);
    _addFact(&_chargeStateFact,             _chargeStateFactName);

    // Start out as not available
    _voltageFact.setRawValue            (_voltageUnavailable);
    _percentRemainingFact.setRawValue   (_percentRemainingUnavailable);
    _mahConsumedFact.setRawValue        (_mahConsumedUnavailable);
    _currentFact.setRawValue            (_currentUnavailable);
    _temperatureFact.setRawValue        (_temperatureUnavailable);
    _cellCountFact.setRawValue          (_cellCountUnavailable);
    _instantPowerFact.setRawValue       (_instantPowerUnavailable);
}

const char* VehicleWindFactGroup::_directionFactName =      "direction";
const char* VehicleWindFactGroup::_speedFactName =          "speed";
const char* VehicleWindFactGroup::_verticalSpeedFactName =  "verticalSpeed";

VehicleWindFactGroup::VehicleWindFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/WindFact.json", parent)
    , _directionFact    (0, _directionFactName,     FactMetaData::valueTypeDouble)
    , _speedFact        (0, _speedFactName,         FactMetaData::valueTypeDouble)
    , _verticalSpeedFact(0, _verticalSpeedFactName, FactMetaData::valueTypeDouble)
{
    _addFact(&_directionFact,       _directionFactName);
    _addFact(&_speedFact,           _speedFactName);
    _addFact(&_verticalSpeedFact,   _verticalSpeedFactName);

    // Start out as not available "--.--"
    _directionFact.setRawValue      (std::numeric_limits<float>::quiet_NaN());
    _speedFact.setRawValue          (std::numeric_limits<float>::quiet_NaN());
    _verticalSpeedFact.setRawValue  (std::numeric_limits<float>::quiet_NaN());
}

const char* VehicleVibrationFactGroup::_xAxisFactName =      "xAxis";
const char* VehicleVibrationFactGroup::_yAxisFactName =      "yAxis";
const char* VehicleVibrationFactGroup::_zAxisFactName =      "zAxis";
const char* VehicleVibrationFactGroup::_clipCount1FactName = "clipCount1";
const char* VehicleVibrationFactGroup::_clipCount2FactName = "clipCount2";
const char* VehicleVibrationFactGroup::_clipCount3FactName = "clipCount3";

VehicleVibrationFactGroup::VehicleVibrationFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/VibrationFact.json", parent)
    , _xAxisFact        (0, _xAxisFactName,         FactMetaData::valueTypeDouble)
    , _yAxisFact        (0, _yAxisFactName,         FactMetaData::valueTypeDouble)
    , _zAxisFact        (0, _zAxisFactName,         FactMetaData::valueTypeDouble)
    , _clipCount1Fact   (0, _clipCount1FactName,    FactMetaData::valueTypeUint32)
    , _clipCount2Fact   (0, _clipCount2FactName,    FactMetaData::valueTypeUint32)
    , _clipCount3Fact   (0, _clipCount3FactName,    FactMetaData::valueTypeUint32)
{
    _addFact(&_xAxisFact,       _xAxisFactName);
    _addFact(&_yAxisFact,       _yAxisFactName);
    _addFact(&_zAxisFact,       _zAxisFactName);
    _addFact(&_clipCount1Fact,  _clipCount1FactName);
    _addFact(&_clipCount2Fact,  _clipCount2FactName);
    _addFact(&_clipCount3Fact,  _clipCount3FactName);

    // Start out as not available "--.--"
    _xAxisFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _yAxisFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _zAxisFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

const char* VehicleTemperatureFactGroup::_temperature1FactName =      "temperature1";
const char* VehicleTemperatureFactGroup::_temperature2FactName =      "temperature2";
const char* VehicleTemperatureFactGroup::_temperature3FactName =      "temperature3";

VehicleTemperatureFactGroup::VehicleTemperatureFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/TemperatureFact.json", parent)
    , _temperature1Fact    (0, _temperature1FactName,     FactMetaData::valueTypeDouble)
    , _temperature2Fact    (0, _temperature2FactName,     FactMetaData::valueTypeDouble)
    , _temperature3Fact    (0, _temperature3FactName,     FactMetaData::valueTypeDouble)
{
    _addFact(&_temperature1Fact,       _temperature1FactName);
    _addFact(&_temperature2Fact,       _temperature2FactName);
    _addFact(&_temperature3Fact,       _temperature3FactName);

    // Start out as not available "--.--"
    _temperature1Fact.setRawValue      (std::numeric_limits<float>::quiet_NaN());
    _temperature2Fact.setRawValue      (std::numeric_limits<float>::quiet_NaN());
    _temperature3Fact.setRawValue      (std::numeric_limits<float>::quiet_NaN());
}

const char* VehicleClockFactGroup::_currentTimeFactName = "currentTime";
const char* VehicleClockFactGroup::_currentDateFactName = "currentDate";

VehicleClockFactGroup::VehicleClockFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/ClockFact.json", parent)
    , _currentTimeFact  (0, _currentTimeFactName,    FactMetaData::valueTypeString)
    , _currentDateFact  (0, _currentDateFactName,    FactMetaData::valueTypeString)
{
    _addFact(&_currentTimeFact, _currentTimeFactName);
    _addFact(&_currentDateFact, _currentDateFactName);

    // Start out as not available "--.--"
    _currentTimeFact.setRawValue    (std::numeric_limits<float>::quiet_NaN());
    _currentDateFact.setRawValue    (std::numeric_limits<float>::quiet_NaN());
}

void VehicleClockFactGroup::_updateAllValues(void)
{
    _currentTimeFact.setRawValue(QTime::currentTime().toString());
    _currentDateFact.setRawValue(QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)));

    FactGroup::_updateAllValues();
}

const char* VehicleSetpointFactGroup::_rollFactName =       "roll";
const char* VehicleSetpointFactGroup::_pitchFactName =      "pitch";
const char* VehicleSetpointFactGroup::_yawFactName =        "yaw";
const char* VehicleSetpointFactGroup::_rollRateFactName =   "rollRate";
const char* VehicleSetpointFactGroup::_pitchRateFactName =  "pitchRate";
const char* VehicleSetpointFactGroup::_yawRateFactName =    "yawRate";

VehicleSetpointFactGroup::VehicleSetpointFactGroup(QObject* parent)
    : FactGroup     (1000, ":/json/Vehicle/SetpointFact.json", parent)
    , _rollFact     (0, _rollFactName,      FactMetaData::valueTypeDouble)
    , _pitchFact    (0, _pitchFactName,     FactMetaData::valueTypeDouble)
    , _yawFact      (0, _yawFactName,       FactMetaData::valueTypeDouble)
    , _rollRateFact (0, _rollRateFactName,  FactMetaData::valueTypeDouble)
    , _pitchRateFact(0, _pitchRateFactName, FactMetaData::valueTypeDouble)
    , _yawRateFact  (0, _yawRateFactName,   FactMetaData::valueTypeDouble)
{
    _addFact(&_rollFact,        _rollFactName);
    _addFact(&_pitchFact,       _pitchFactName);
    _addFact(&_yawFact,         _yawFactName);
    _addFact(&_rollRateFact,    _rollRateFactName);
    _addFact(&_pitchRateFact,   _pitchRateFactName);
    _addFact(&_yawRateFact,     _yawRateFactName);

    // Start out as not available "--.--"
    _rollFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _pitchFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _yawFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rollRateFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _pitchRateFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _yawRateFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

const char* VehicleDistanceSensorFactGroup::_rotationNoneFactName =     "rotationNone";
const char* VehicleDistanceSensorFactGroup::_rotationYaw45FactName =    "rotationYaw45";
const char* VehicleDistanceSensorFactGroup::_rotationYaw90FactName =    "rotationYaw90";
const char* VehicleDistanceSensorFactGroup::_rotationYaw135FactName =   "rotationYaw135";
const char* VehicleDistanceSensorFactGroup::_rotationYaw180FactName =   "rotationYaw180";
const char* VehicleDistanceSensorFactGroup::_rotationYaw225FactName =   "rotationYaw225";
const char* VehicleDistanceSensorFactGroup::_rotationYaw270FactName =   "rotationYaw270";
const char* VehicleDistanceSensorFactGroup::_rotationYaw315FactName =   "rotationYaw315";
const char* VehicleDistanceSensorFactGroup::_rotationPitch90FactName =  "rotationPitch90";
const char* VehicleDistanceSensorFactGroup::_rotationPitch270FactName = "rotationPitch270";

VehicleDistanceSensorFactGroup::VehicleDistanceSensorFactGroup(QObject* parent)
    : FactGroup             (1000, ":/json/Vehicle/DistanceSensorFact.json", parent)
    , _rotationNoneFact     (0, _rotationNoneFactName,      FactMetaData::valueTypeDouble)
    , _rotationYaw45Fact    (0, _rotationYaw45FactName,     FactMetaData::valueTypeDouble)
    , _rotationYaw90Fact    (0, _rotationYaw90FactName,     FactMetaData::valueTypeDouble)
    , _rotationYaw135Fact   (0, _rotationYaw135FactName,    FactMetaData::valueTypeDouble)
    , _rotationYaw180Fact   (0, _rotationYaw180FactName,    FactMetaData::valueTypeDouble)
    , _rotationYaw225Fact   (0, _rotationYaw225FactName,    FactMetaData::valueTypeDouble)
    , _rotationYaw270Fact   (0, _rotationYaw270FactName,    FactMetaData::valueTypeDouble)
    , _rotationYaw315Fact   (0, _rotationYaw315FactName,    FactMetaData::valueTypeDouble)
    , _rotationPitch90Fact  (0, _rotationPitch90FactName,   FactMetaData::valueTypeDouble)
    , _rotationPitch270Fact (0, _rotationPitch270FactName,  FactMetaData::valueTypeDouble)
    , _idSet                (false)
    , _id                   (0)
{
    _addFact(&_rotationNoneFact,        _rotationNoneFactName);
    _addFact(&_rotationYaw45Fact,       _rotationYaw45FactName);
    _addFact(&_rotationYaw90Fact,       _rotationYaw90FactName);
    _addFact(&_rotationYaw135Fact,      _rotationYaw135FactName);
    _addFact(&_rotationYaw180Fact,      _rotationYaw180FactName);
    _addFact(&_rotationYaw225Fact,      _rotationYaw225FactName);
    _addFact(&_rotationYaw270Fact,      _rotationYaw270FactName);
    _addFact(&_rotationYaw315Fact,      _rotationYaw315FactName);
    _addFact(&_rotationPitch90Fact,     _rotationPitch90FactName);
    _addFact(&_rotationPitch270Fact,    _rotationPitch270FactName);

    // Start out as not available "--.--"
    _rotationNoneFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rotationYaw45Fact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rotationYaw135Fact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rotationYaw90Fact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rotationYaw180Fact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rotationYaw225Fact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rotationYaw270Fact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rotationPitch90Fact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rotationPitch270Fact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}
