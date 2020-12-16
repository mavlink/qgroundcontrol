/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QTime>
#include <QDateTime>
#include <QLocale>
#include <QQuaternion>

#include <Eigen/Eigen>

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
#include "FlightPathSegment.h"
#include "QGCApplication.h"
#include "QGCImageProvider.h"
#include "MissionCommandTree.h"
#include "SettingsManager.h"
#include "QGCQGeoCoordinate.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "ADSBVehicleManager.h"
#include "QGCCameraManager.h"
#include "VideoReceiver.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "PositionManager.h"
#include "VehicleObjectAvoidance.h"
#include "TrajectoryPoints.h"
#include "QGCGeo.h"
#include "TerrainProtocolHandler.h"
#include "ParameterManager.h"
#include "FTPManager.h"
#include "ComponentInformationManager.h"
#include "InitialConnectStateMachine.h"
#include "VehicleBatteryFactGroup.h"
#ifdef QT_DEBUG
#include "MockLink.h"
#endif

#if defined(QGC_AIRMAP_ENABLED)
#include "AirspaceVehicleManager.h"
#endif

QGC_LOGGING_CATEGORY(VehicleLog, "VehicleLog")

#define UPDATE_TIMER 50
#define DEFAULT_LAT  38.965767f
#define DEFAULT_LON -120.083923f

const QString guided_mode_not_supported_by_vehicle = QObject::tr("Guided mode not supported by Vehicle.");

const char* Vehicle::_settingsGroup =               "Vehicle%1";        // %1 replaced with mavlink system id
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
const char* Vehicle::_missionItemIndexFactName =    "missionItemIndex";
const char* Vehicle::_headingToNextWPFactName =     "headingToNextWP";
const char* Vehicle::_headingToHomeFactName =       "headingToHome";
const char* Vehicle::_distanceToGCSFactName =       "distanceToGCS";
const char* Vehicle::_hobbsFactName =               "hobbs";
const char* Vehicle::_throttlePctFactName =         "throttlePct";

const char* Vehicle::_gpsFactGroupName =                "gps";
const char* Vehicle::_windFactGroupName =               "wind";
const char* Vehicle::_vibrationFactGroupName =          "vibration";
const char* Vehicle::_temperatureFactGroupName =        "temperature";
const char* Vehicle::_clockFactGroupName =              "clock";
const char* Vehicle::_setpointFactGroupName =           "setpoint";
const char* Vehicle::_distanceSensorFactGroupName =     "distanceSensor";
const char* Vehicle::_escStatusFactGroupName =          "escStatus";
const char* Vehicle::_estimatorStatusFactGroupName =    "estimatorStatus";
const char* Vehicle::_terrainFactGroupName =            "terrain";

const QList<int> Vehicle::_pidTuningMessages = {MAVLINK_MSG_ID_ATTITUDE_QUATERNION, MAVLINK_MSG_ID_ATTITUDE_TARGET};

// Standard connected vehicle
Vehicle::Vehicle(LinkInterface*             link,
                 int                        vehicleId,
                 int                        defaultComponentId,
                 MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 JoystickManager*           joystickManager)
    : FactGroup                     (_vehicleUIUpdateRateMSecs, ":/json/Vehicle/VehicleFact.json")
    , _id                           (vehicleId)
    , _defaultComponentId           (defaultComponentId)
    , _firmwareType                 (firmwareType)
    , _vehicleType                  (vehicleType)
    , _toolbox                      (qgcApp()->toolbox())
    , _settingsManager              (_toolbox->settingsManager())
    , _defaultCruiseSpeed           (_settingsManager->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed            (_settingsManager->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _firmwarePluginManager        (firmwarePluginManager)
    , _joystickManager              (joystickManager)
    , _trajectoryPoints             (new TrajectoryPoints(this, this))
    , _rollFact                     (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact                    (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact                  (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _rollRateFact                 (0, _rollRateFactName,          FactMetaData::valueTypeDouble)
    , _pitchRateFact                (0, _pitchRateFactName,         FactMetaData::valueTypeDouble)
    , _yawRateFact                  (0, _yawRateFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact              (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact                 (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _climbRateFact                (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact         (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact             (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _flightDistanceFact           (0, _flightDistanceFactName,    FactMetaData::valueTypeDouble)
    , _flightTimeFact               (0, _flightTimeFactName,        FactMetaData::valueTypeElapsedTimeInSeconds)
    , _distanceToHomeFact           (0, _distanceToHomeFactName,    FactMetaData::valueTypeDouble)
    , _missionItemIndexFact         (0, _missionItemIndexFactName,  FactMetaData::valueTypeUint16)
    , _headingToNextWPFact          (0, _headingToNextWPFactName,   FactMetaData::valueTypeDouble)
    , _headingToHomeFact            (0, _headingToHomeFactName,     FactMetaData::valueTypeDouble)
    , _distanceToGCSFact            (0, _distanceToGCSFactName,     FactMetaData::valueTypeDouble)
    , _hobbsFact                    (0, _hobbsFactName,             FactMetaData::valueTypeString)
    , _throttlePctFact              (0, _throttlePctFactName,       FactMetaData::valueTypeUint16)
    , _gpsFactGroup                 (this)
    , _windFactGroup                (this)
    , _vibrationFactGroup           (this)
    , _temperatureFactGroup         (this)
    , _clockFactGroup               (this)
    , _setpointFactGroup            (this)
    , _distanceSensorFactGroup      (this)
    , _escStatusFactGroup           (this)
    , _estimatorStatusFactGroup     (this)
    , _terrainFactGroup             (this)
    , _terrainProtocolHandler       (new TerrainProtocolHandler(this, &_terrainFactGroup, this))
{
    _linkManager = _toolbox->linkManager();

    connect(_joystickManager, &JoystickManager::activeJoystickChanged, this, &Vehicle::_loadSettings);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleAvailableChanged, this, &Vehicle::_loadSettings);

    _mavlink = _toolbox->mavlinkProtocol();
    qCDebug(VehicleLog) << "Link started with Mavlink " << (_mavlink->getCurrentVersion() >= 200 ? "V2" : "V1");

    connect(_mavlink, &MAVLinkProtocol::messageReceived,        this, &Vehicle::_mavlinkMessageReceived);
    connect(_mavlink, &MAVLinkProtocol::mavlinkMessageStatus,   this, &Vehicle::_mavlinkMessageStatus);

    connect(this, &Vehicle::flightModeChanged,          this, &Vehicle::_handleFlightModeChanged);
    connect(this, &Vehicle::armedChanged,               this, &Vehicle::_announceArmedChanged);

    connect(_toolbox->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &Vehicle::_vehicleParamLoaded);

    _uas = new UAS(_mavlink, this, _firmwarePluginManager);
    _uas->setParent(this);

    connect(_uas, &UAS::imageReady,                     this, &Vehicle::_imageReady);
    connect(this, &Vehicle::remoteControlRSSIChanged,   this, &Vehicle::_remoteControlRSSIChanged);

    _commonInit();

    _vehicleLinkManager->_addLink(link);

    // Set video stream to udp if running ArduSub and Video is disabled
    if (sub() && _settingsManager->videoSettings()->videoSource()->rawValue() == VideoSettings::videoDisabled) {
        _settingsManager->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
        _settingsManager->videoSettings()->lowLatencyMode()->setRawValue(true);
    }

    //-- Airspace Management
#if defined(QGC_AIRMAP_ENABLED)
    AirspaceManager* airspaceManager = _toolbox->airspaceManager();
    if (airspaceManager) {
        _airspaceVehicleManager = airspaceManager->instantiateVehicle(*this);
        if (_airspaceVehicleManager) {
            connect(_airspaceVehicleManager, &AirspaceVehicleManager::trafficUpdate, this, &Vehicle::_trafficUpdate);
        }
    }
#endif

    _autopilotPlugin = _firmwarePlugin->autopilotPlugin(this);
    _autopilotPlugin->setParent(this);

    // PreArm Error self-destruct timer
    connect(&_prearmErrorTimer, &QTimer::timeout, this, &Vehicle::_prearmErrorTimeout);
    _prearmErrorTimer.setInterval(_prearmErrorTimeoutMSecs);
    _prearmErrorTimer.setSingleShot(true);

    // Send MAV_CMD ack timer
    _mavCommandResponseCheckTimer.setSingleShot(false);
    _mavCommandResponseCheckTimer.setInterval(_mavCommandResponseCheckTimeoutMSecs);
    _mavCommandResponseCheckTimer.start();
    connect(&_mavCommandResponseCheckTimer, &QTimer::timeout, this, &Vehicle::_sendMavCommandResponseTimeoutCheck);

    // Chunked status text timeout timer
    _chunkedStatusTextTimer.setSingleShot(true);
    _chunkedStatusTextTimer.setInterval(1000);
    connect(&_chunkedStatusTextTimer, &QTimer::timeout, this, &Vehicle::_chunkedStatusTextTimeout);

    _mav = uas();

    // Listen for system messages
    connect(_toolbox->uasMessageHandler(), &UASMessageHandler::textMessageCountChanged,  this, &Vehicle::_handleTextMessage);
    connect(_toolbox->uasMessageHandler(), &UASMessageHandler::textMessageReceived,      this, &Vehicle::_handletextMessageReceived);

    // MAV_TYPE_GENERIC is used by unit test for creating a vehicle which doesn't do the connect sequence. This
    // way we can test the methods that are used within the connect sequence.
    if (!qgcApp()->runningUnitTests() || _vehicleType != MAV_TYPE_GENERIC) {
        _initialConnectStateMachine->start();
    }

    _firmwarePlugin->initializeVehicle(this);
    for(auto& factName: factNames()) {
        _firmwarePlugin->adjustMetaData(vehicleType, getFact(factName)->metaData());
    }

    _sendMultipleTimer.start(_sendMessageMultipleIntraMessageDelay);
    connect(&_sendMultipleTimer, &QTimer::timeout, this, &Vehicle::_sendMessageMultipleNext);

    connect(&_orbitTelemetryTimer, &QTimer::timeout, this, &Vehicle::_orbitTelemetryTimeout);

    // Create camera manager instance
    _cameraManager = _firmwarePlugin->createCameraManager(this);
    emit cameraManagerChanged();

    // Start csv logger
    connect(&_csvLogTimer, &QTimer::timeout, this, &Vehicle::_writeCsvLine);
    _csvLogTimer.start(1000);
}

// Disconnected Vehicle for offline editing
Vehicle::Vehicle(MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 QObject*                   parent)
    : FactGroup                         (_vehicleUIUpdateRateMSecs, ":/json/Vehicle/VehicleFact.json", parent)
    , _id                               (0)
    , _defaultComponentId               (MAV_COMP_ID_ALL)
    , _offlineEditingVehicle            (true)
    , _firmwareType                     (firmwareType)
    , _vehicleType                      (vehicleType)
    , _toolbox                          (qgcApp()->toolbox())
    , _settingsManager                  (_toolbox->settingsManager())
    , _defaultCruiseSpeed               (_settingsManager->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed                (_settingsManager->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _mavlinkProtocolRequestComplete   (true)
    , _maxProtoVersion                  (200)
    , _capabilityBitsKnown              (true)
    , _capabilityBits                   (MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY)
    , _firmwarePluginManager            (firmwarePluginManager)
    , _trajectoryPoints                 (new TrajectoryPoints(this, this))
    , _rollFact                         (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact                        (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact                      (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _rollRateFact                     (0, _rollRateFactName,          FactMetaData::valueTypeDouble)
    , _pitchRateFact                    (0, _pitchRateFactName,         FactMetaData::valueTypeDouble)
    , _yawRateFact                      (0, _yawRateFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact                  (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact                     (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _climbRateFact                    (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact             (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact                 (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _flightDistanceFact               (0, _flightDistanceFactName,    FactMetaData::valueTypeDouble)
    , _flightTimeFact                   (0, _flightTimeFactName,        FactMetaData::valueTypeElapsedTimeInSeconds)
    , _distanceToHomeFact               (0, _distanceToHomeFactName,    FactMetaData::valueTypeDouble)
    , _missionItemIndexFact             (0, _missionItemIndexFactName,  FactMetaData::valueTypeUint16)
    , _headingToNextWPFact              (0, _headingToNextWPFactName,   FactMetaData::valueTypeDouble)
    , _headingToHomeFact                (0, _headingToHomeFactName,     FactMetaData::valueTypeDouble)
    , _distanceToGCSFact                (0, _distanceToGCSFactName,     FactMetaData::valueTypeDouble)
    , _hobbsFact                        (0, _hobbsFactName,             FactMetaData::valueTypeString)
    , _throttlePctFact                  (0, _throttlePctFactName,       FactMetaData::valueTypeUint16)
    , _gpsFactGroup                     (this)
    , _windFactGroup                    (this)
    , _vibrationFactGroup               (this)
    , _clockFactGroup                   (this)
    , _distanceSensorFactGroup          (this)
{
    _linkManager = _toolbox->linkManager();

    // This will also set the settings based firmware/vehicle types. So it needs to happen first.
    if (_firmwareType == MAV_AUTOPILOT_TRACK) {
        trackFirmwareVehicleTypeChanges();
    }

    _commonInit();

    connect(_settingsManager->appSettings()->offlineEditingCruiseSpeed(),   &Fact::rawValueChanged, this, &Vehicle::_offlineCruiseSpeedSettingChanged);
    connect(_settingsManager->appSettings()->offlineEditingHoverSpeed(),    &Fact::rawValueChanged, this, &Vehicle::_offlineHoverSpeedSettingChanged);

    _offlineFirmwareTypeSettingChanged(_firmwareType);  // This adds correct terrain capability bit
    _firmwarePlugin->initializeVehicle(this);
}

void Vehicle::trackFirmwareVehicleTypeChanges(void)
{
    connect(_settingsManager->appSettings()->offlineEditingFirmwareClass(), &Fact::rawValueChanged, this, &Vehicle::_offlineFirmwareTypeSettingChanged);
    connect(_settingsManager->appSettings()->offlineEditingVehicleClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineVehicleTypeSettingChanged);

    _offlineFirmwareTypeSettingChanged(_settingsManager->appSettings()->offlineEditingFirmwareClass()->rawValue());
    _offlineVehicleTypeSettingChanged(_settingsManager->appSettings()->offlineEditingVehicleClass()->rawValue());
}

void Vehicle::stopTrackingFirmwareVehicleTypeChanges(void)
{
    disconnect(_settingsManager->appSettings()->offlineEditingFirmwareClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineFirmwareTypeSettingChanged);
    disconnect(_settingsManager->appSettings()->offlineEditingVehicleClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineVehicleTypeSettingChanged);
}

void Vehicle::_commonInit()
{
    _firmwarePlugin = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);

    connect(_firmwarePlugin, &FirmwarePlugin::toolIndicatorsChanged, this, &Vehicle::toolIndicatorsChanged);
    connect(_firmwarePlugin, &FirmwarePlugin::modeIndicatorsChanged, this, &Vehicle::modeIndicatorsChanged);

    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateDistanceHeadingToHome);
    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateDistanceToGCS);
    connect(this, &Vehicle::homePositionChanged,    this, &Vehicle::_updateDistanceHeadingToHome);
    connect(this, &Vehicle::hobbsMeterChanged,      this, &Vehicle::_updateHobbsMeter);

    connect(_toolbox->qgcPositionManager(), &QGCPositionManager::gcsPositionChanged, this, &Vehicle::_updateDistanceToGCS);

    _missionManager = new MissionManager(this);
    connect(_missionManager, &MissionManager::error,                    this, &Vehicle::_missionManagerError);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_firstMissionLoadComplete);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_clearCameraTriggerPoints);
    connect(_missionManager, &MissionManager::sendComplete,             this, &Vehicle::_clearCameraTriggerPoints);
    connect(_missionManager, &MissionManager::currentIndexChanged,      this, &Vehicle::_updateHeadingToNextWP);
    connect(_missionManager, &MissionManager::currentIndexChanged,      this, &Vehicle::_updateMissionItemIndex);

    connect(_missionManager, &MissionManager::sendComplete,             _trajectoryPoints, &TrajectoryPoints::clear);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, _trajectoryPoints, &TrajectoryPoints::clear);

    _componentInformationManager    = new ComponentInformationManager   (this);
    _initialConnectStateMachine     = new InitialConnectStateMachine    (this);
    _ftpManager                     = new FTPManager                    (this);    
    _vehicleLinkManager             = new VehicleLinkManager            (this);

    _parameterManager = new ParameterManager(this);
    connect(_parameterManager, &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);

    _objectAvoidance = new VehicleObjectAvoidance(this, this);

    // GeoFenceManager needs to access ParameterManager so make sure to create after
    _geoFenceManager = new GeoFenceManager(this);
    connect(_geoFenceManager, &GeoFenceManager::error,          this, &Vehicle::_geoFenceManagerError);
    connect(_geoFenceManager, &GeoFenceManager::loadComplete,   this, &Vehicle::_firstGeoFenceLoadComplete);

    _rallyPointManager = new RallyPointManager(this);
    connect(_rallyPointManager, &RallyPointManager::error,          this, &Vehicle::_rallyPointManagerError);
    connect(_rallyPointManager, &RallyPointManager::loadComplete,   this, &Vehicle::_firstRallyPointLoadComplete);

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
    _addFact(&_missionItemIndexFact,    _missionItemIndexFactName);
    _addFact(&_headingToNextWPFact,     _headingToNextWPFactName);
    _addFact(&_headingToHomeFact,       _headingToHomeFactName);
    _addFact(&_distanceToGCSFact,       _distanceToGCSFactName);
    _addFact(&_throttlePctFact,         _throttlePctFactName);

    _hobbsFact.setRawValue(QVariant(QString("0000:00:00")));
    _addFact(&_hobbsFact,               _hobbsFactName);

    _addFactGroup(&_gpsFactGroup,               _gpsFactGroupName);
    _addFactGroup(&_windFactGroup,              _windFactGroupName);
    _addFactGroup(&_vibrationFactGroup,         _vibrationFactGroupName);
    _addFactGroup(&_temperatureFactGroup,       _temperatureFactGroupName);
    _addFactGroup(&_clockFactGroup,             _clockFactGroupName);
    _addFactGroup(&_setpointFactGroup,          _setpointFactGroupName);
    _addFactGroup(&_distanceSensorFactGroup,    _distanceSensorFactGroupName);
    _addFactGroup(&_escStatusFactGroup,         _escStatusFactGroupName);
    _addFactGroup(&_estimatorStatusFactGroup,   _estimatorStatusFactGroupName);
    _addFactGroup(&_terrainFactGroup,           _terrainFactGroupName);

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
    _flightTimeUpdater.setInterval(1000);
    _flightTimeUpdater.setSingleShot(false);
    connect(&_flightTimeUpdater, &QTimer::timeout, this, &Vehicle::_updateFlightTime);

    // Set video stream to udp if running ArduSub and Video is disabled
    if (sub() && _settingsManager->videoSettings()->videoSource()->rawValue() == VideoSettings::videoDisabled) {
        _settingsManager->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
        _settingsManager->videoSettings()->lowLatencyMode()->setRawValue(true);
    }

    //-- Airspace Management
#if defined(QGC_AIRMAP_ENABLED)
    AirspaceManager* airspaceManager = _toolbox->airspaceManager();
    if (airspaceManager) {
        _airspaceVehicleManager = airspaceManager->instantiateVehicle(*this);
        if (_airspaceVehicleManager) {
            connect(_airspaceVehicleManager, &AirspaceVehicleManager::trafficUpdate, this, &Vehicle::_trafficUpdate);
        }
    }
#endif
}

Vehicle::~Vehicle()
{
    qCDebug(VehicleLog) << "~Vehicle" << this;

    delete _missionManager;
    _missionManager = nullptr;

    delete _autopilotPlugin;
    _autopilotPlugin = nullptr;

    delete _mav;
    _mav = nullptr;

#if defined(QGC_AIRMAP_ENABLED)
    if (_airspaceVehicleManager) {
        delete _airspaceVehicleManager;
    }
#endif
}

void Vehicle::prepareDelete()
{
    if(_cameraManager) {
        // because of _cameraManager QML bindings check for nullptr won't work in the binding pipeline
        // the dangling pointer access will cause a runtime fault
        auto tmpCameras = _cameraManager;
        _cameraManager = nullptr;
        delete tmpCameras;
        emit cameraManagerChanged();
        qApp->processEvents();
    }
}

void Vehicle::_offlineFirmwareTypeSettingChanged(QVariant varFirmwareType)
{
    _firmwareType = static_cast<MAV_AUTOPILOT>(varFirmwareType.toInt());
    _firmwarePlugin = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);
    if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        _capabilityBits |= MAV_PROTOCOL_CAPABILITY_TERRAIN;
    } else {
        _capabilityBits &= ~MAV_PROTOCOL_CAPABILITY_TERRAIN;
    }
    emit firmwareTypeChanged();
    emit capabilityBitsChanged(_capabilityBits);
}

void Vehicle::_offlineVehicleTypeSettingChanged(QVariant varVehicleType)
{
    _vehicleType = static_cast<MAV_TYPE>(varVehicleType.toInt());
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

QString Vehicle::firmwareTypeString() const
{
    if (px4Firmware()) {
        return QStringLiteral("PX4 Pro");
    } else if (apmFirmware()) {
        return QStringLiteral("ArduPilot");
    } else {
        return tr("MAVLink Generic");
    }
}

QString Vehicle::vehicleTypeString() const
{
    if (airship()) {
        return tr("Airship");
    } else if (fixedWing()) {
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
    // If the link is already running at Mavlink V2 set our max proto version to it.
    unsigned mavlinkVersion = _mavlink->getCurrentVersion();
    if (_maxProtoVersion != mavlinkVersion && mavlinkVersion >= 200) {
        _maxProtoVersion = mavlinkVersion;
        qCDebug(VehicleLog) << "Vehicle::_mavlinkMessageReceived Link already running Mavlink v2. Setting _maxProtoVersion" << _maxProtoVersion;
    }

    if (message.sysid != _id && message.sysid != 0) {
        // We allow RADIO_STATUS messages which come from a link the vehicle is using to pass through and be handled
        if (!(message.msgid == MAVLINK_MSG_ID_RADIO_STATUS && _vehicleLinkManager->containsLink(link))) {
            return;
        }
    }

    // We give the link manager first whack since it it reponsible for adding new links
    _vehicleLinkManager->mavlinkMessageReceived(link, message);

    //-- Check link status
    _messagesReceived++;
    emit messagesReceivedChanged();
    if(!_heardFrom) {
        if(message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            _heardFrom  = true;
            _compID     = message.compid;
            _messageSeq = message.seq + 1;
        }
    } else {
        if(_compID == message.compid) {
            uint16_t seq_received = static_cast<uint16_t>(message.seq);
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

    if (!_terrainProtocolHandler->mavlinkMessageReceived(message)) {
        return;
    }
    _ftpManager->_mavlinkMessageReceived(message);
    _parameterManager->mavlinkMessageReceived(message);

    _waitForMavlinkMessageMessageReceived(message);

    // Battery fact groups are created dynamically as new batteries are discovered
    VehicleBatteryFactGroup::handleMessageForFactGroupCreation(this, message);

    // Let the fact groups take a whack at the mavlink traffic
    for (FactGroup* factGroup : factGroups()) {
        factGroup->handleMessage(this, message);
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
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE:
        _handleExtendedSysState(message);
        break;
    case MAVLINK_MSG_ID_COMMAND_ACK:
        _handleCommandAck(message);
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
    case MAVLINK_MSG_ID_CAMERA_IMAGE_CAPTURED:
        _handleCameraImageCaptured(message);
        break;
    case MAVLINK_MSG_ID_ADSB_VEHICLE:
        _handleADSBVehicle(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(message);
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
    case MAVLINK_MSG_ID_STATUSTEXT:
        _handleStatusText(message);
        break;
    case MAVLINK_MSG_ID_ORBIT_EXECUTION_STATUS:
        _handleOrbitExecutionStatus(message);
        break;
    case MAVLINK_MSG_ID_MESSAGE_INTERVAL:
        _handleMessageInterval(message);
        break;
    case MAVLINK_MSG_ID_PING:
        _handlePing(link, message);
        break;
    case MAVLINK_MSG_ID_MOUNT_ORIENTATION:
        _handleGimbalOrientation(message);
        break;
    case MAVLINK_MSG_ID_OBSTACLE_DISTANCE:
        _handleObstacleDistance(message);
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

void Vehicle::_handleOrbitExecutionStatus(const mavlink_message_t& message)
{
    mavlink_orbit_execution_status_t orbitStatus;

    mavlink_msg_orbit_execution_status_decode(&message, &orbitStatus);

    double newRadius =  qAbs(static_cast<double>(orbitStatus.radius));
    if (!QGC::fuzzyCompare(_orbitMapCircle.radius()->rawValue().toDouble(), newRadius)) {
        _orbitMapCircle.radius()->setRawValue(newRadius);
    }

    bool newOrbitClockwise = orbitStatus.radius > 0 ? true : false;
    if (_orbitMapCircle.clockwiseRotation() != newOrbitClockwise) {
        _orbitMapCircle.setClockwiseRotation(newOrbitClockwise);
    }

    QGeoCoordinate newCenter(static_cast<double>(orbitStatus.x) / qPow(10.0, 7.0), static_cast<double>(orbitStatus.y) / qPow(10.0, 7.0));
    if (_orbitMapCircle.center() != newCenter) {
        _orbitMapCircle.setCenter(newCenter);
    }

    if (!_orbitActive) {
        _orbitActive = true;
        _orbitMapCircle.setShowRotation(true);
        emit orbitActiveChanged(true);
    }

    _orbitTelemetryTimer.start(_orbitTelemetryTimeoutMsecs);
}

void Vehicle::_orbitTelemetryTimeout()
{
    _orbitActive = false;
    emit orbitActiveChanged(false);
}

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

void Vehicle::_chunkedStatusTextTimeout(void)
{
    // Spit out all incomplete chunks
    QList<uint8_t> rgCompId = _chunkedStatusTextInfoMap.keys();
    for (uint8_t compId : rgCompId) {
        _chunkedStatusTextInfoMap[compId].rgMessageChunks.append(QString());
        _chunkedStatusTextCompleted(compId);
    }
}

void Vehicle::_chunkedStatusTextCompleted(uint8_t compId)
{
    ChunkedStatusTextInfo_t&    chunkedInfo =   _chunkedStatusTextInfoMap[compId];
    uint8_t                     severity =      chunkedInfo.severity;
    QStringList&                rgChunks =      chunkedInfo.rgMessageChunks;

    // Build up message from chunks
    QString messageText;
    for (const QString& chunk : rgChunks) {
        if (chunk.isEmpty()) {
            // Indicates missing chunk
            messageText += tr(" ... ", "Indicates missing chunk from chunked STATUS_TEXT");
        } else {
            messageText += chunk;
        }
    }

    _chunkedStatusTextInfoMap.remove(compId);

    bool skipSpoken = false;
    bool ardupilotPrearm = messageText.startsWith(QStringLiteral("PreArm"));
    bool px4Prearm = messageText.startsWith(QStringLiteral("preflight"), Qt::CaseInsensitive) && severity >= MAV_SEVERITY_CRITICAL;
    if (ardupilotPrearm || px4Prearm) {
        // Limit repeated PreArm message to once every 10 seconds
        if (_noisySpokenPrearmMap.contains(messageText) && _noisySpokenPrearmMap[messageText].msecsTo(QTime::currentTime()) < (10 * 1000)) {
            skipSpoken = true;
        } else {
            _noisySpokenPrearmMap[messageText] = QTime::currentTime();
            setPrearmError(messageText);
        }
    }

    // If the message is NOTIFY or higher severity, or starts with a '#',
    // then read it aloud.
    bool readAloud = false;

    if (messageText.startsWith("#")) {
        messageText.remove(0, 1);
        readAloud = true;
    }
    else if (severity <= MAV_SEVERITY_NOTICE) {
        readAloud = true;
    }

    if (readAloud) {
        if (!skipSpoken) {
            qgcApp()->toolbox()->audioOutput()->say(messageText);
        }
    }
    emit textMessageReceived(id(), compId, severity, messageText);
}

void Vehicle::_handleStatusText(mavlink_message_t& message)
{
    QByteArray  b;
    QString     messageText;

    mavlink_statustext_t statustext;
    mavlink_msg_statustext_decode(&message, &statustext);

    uint8_t compId = message.compid;

    b.resize(MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN+1);
    strncpy(b.data(), statustext.text, MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN);
    b[b.length()-1] = '\0';
    messageText = QString(b);
    bool includesNullTerminator = messageText.length() < MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN;

    if (_chunkedStatusTextInfoMap.contains(compId) && _chunkedStatusTextInfoMap[compId].chunkId != statustext.id) {
        // We have an incomplete chunked status still pending
        _chunkedStatusTextInfoMap[compId].rgMessageChunks.append(QString());
        _chunkedStatusTextCompleted(compId);
    }

    if (statustext.id == 0) {
        // Non-chunked status text. We still use common chunked text output mechanism.
        ChunkedStatusTextInfo_t chunkedInfo;
        chunkedInfo.chunkId = 0;
        chunkedInfo.severity = statustext.severity;
        chunkedInfo.rgMessageChunks.append(messageText);
        _chunkedStatusTextInfoMap[compId] = chunkedInfo;
    } else {
        if (_chunkedStatusTextInfoMap.contains(compId)) {
            // A chunk sequence is in progress
            QStringList& chunks = _chunkedStatusTextInfoMap[compId].rgMessageChunks;
            if (statustext.chunk_seq > chunks.size()) {
                // We are missing some chunks in between, fill them in as missing
                for (int i=chunks.size(); i<statustext.chunk_seq; i++) {
                    chunks.append(QString());
                }
            }
            chunks.append(messageText);
        } else {
            // Starting a new chunk sequence
            ChunkedStatusTextInfo_t chunkedInfo;
            chunkedInfo.chunkId = statustext.id;
            chunkedInfo.severity = statustext.severity;
            chunkedInfo.rgMessageChunks.append(messageText);
            _chunkedStatusTextInfoMap[compId] = chunkedInfo;
        }
        _chunkedStatusTextTimer.start();
    }

    if (statustext.id == 0 || includesNullTerminator) {
        _chunkedStatusTextTimer.stop();
        _chunkedStatusTextCompleted(message.compid);
    }
}

void Vehicle::_handleVfrHud(mavlink_message_t& message)
{
    mavlink_vfr_hud_t vfrHud;
    mavlink_msg_vfr_hud_decode(&message, &vfrHud);

    _airSpeedFact.setRawValue(qIsNaN(vfrHud.airspeed) ? 0 : vfrHud.airspeed);
    _groundSpeedFact.setRawValue(qIsNaN(vfrHud.groundspeed) ? 0 : vfrHud.groundspeed);
    _climbRateFact.setRawValue(qIsNaN(vfrHud.climb) ? 0 : vfrHud.climb);
    _throttlePctFact.setRawValue(static_cast<int16_t>(vfrHud.throttle));
}

// Ignore warnings from mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__

#if __GNUC__ > 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-packed-member"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#endif

#else
#pragma warning(push, 0)
#endif

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

    Eigen::Quaternionf quat(attitudeQuaternion.q1, attitudeQuaternion.q2, attitudeQuaternion.q3, attitudeQuaternion.q4);
    Eigen::Vector3f rates(attitudeQuaternion.rollspeed, attitudeQuaternion.pitchspeed, attitudeQuaternion.yawspeed);
    Eigen::Quaternionf repr_offset(attitudeQuaternion.repr_offset_q[0], attitudeQuaternion.repr_offset_q[1], attitudeQuaternion.repr_offset_q[2], attitudeQuaternion.repr_offset_q[3]);

    // if repr_offset is valid, rotate attitude and rates
    if (repr_offset.norm() >= 0.5f) {
        quat = quat * repr_offset;
        rates = repr_offset * rates;
    }

    float roll, pitch, yaw;
    float q[] = { quat.w(), quat.x(), quat.y(), quat.z() };
    mavlink_quaternion_to_euler(q, &roll, &pitch, &yaw);

    _handleAttitudeWorker(roll, pitch, yaw);

    rollRate()->setRawValue(qRadiansToDegrees(rates[0]));
    pitchRate()->setRawValue(qRadiansToDegrees(rates[1]));
    yawRate()->setRawValue(qRadiansToDegrees(rates[2]));
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
            if (!_altitudeMessageAvailable) {
                _altitudeAMSLFact.setRawValue(gpsRawInt.alt / 1000.0);
            }
        }
    }
}

void Vehicle::_handleGlobalPositionInt(mavlink_message_t& message)
{
    mavlink_global_position_int_t globalPositionInt;
    mavlink_msg_global_position_int_decode(&message, &globalPositionInt);

    if (!_altitudeMessageAvailable) {
        _altitudeRelativeFact.setRawValue(globalPositionInt.relative_alt / 1000.0);
        _altitudeAMSLFact.setRawValue(globalPositionInt.alt / 1000.0);
    }

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

void Vehicle::_handleHighLatency(mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);

    QString previousFlightMode;
    if (_base_mode != 0 || _custom_mode != 0){
        // Vehicle is initialized with _base_mode=0 and _custom_mode=0. Don't pass this to flightMode() since it will complain about
        // bad modes while unit testing.
        previousFlightMode = flightMode();
    }
    _base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    _custom_mode = _firmwarePlugin->highLatencyCustomModeTo32Bits(highLatency.custom_mode);
    if (previousFlightMode != flightMode()) {
        emit flightModeChanged(flightMode());
    }

    // Assume armed since we don't know
    if (_armed != true) {
        _armed = true;
        emit armedChanged(_armed);
    }

    struct {
        const double latitude;
        const double longitude;
        const double altitude;
    } coordinate {
        highLatency.latitude  / (double)1E7,
                highLatency.longitude  / (double)1E7,
                static_cast<double>(highLatency.altitude_amsl)
    };

    _coordinate.setLatitude(coordinate.latitude);
    _coordinate.setLongitude(coordinate.longitude);
    _coordinate.setAltitude(coordinate.altitude);
    emit coordinateChanged(_coordinate);

    _airSpeedFact.setRawValue((double)highLatency.airspeed / 5.0);
    _groundSpeedFact.setRawValue((double)highLatency.groundspeed / 5.0);
    _climbRateFact.setRawValue((double)highLatency.climb_rate / 10.0);
    _headingFact.setRawValue((double)highLatency.heading * 2.0);
    _altitudeRelativeFact.setRawValue(qQNaN());
    _altitudeAMSLFact.setRawValue(coordinate.altitude);
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
    _altitudeRelativeFact.setRawValue(qQNaN());
    _altitudeAMSLFact.setRawValue(highLatency2.altitude);

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
    }
}

void Vehicle::_handleAltitude(mavlink_message_t& message)
{
    mavlink_altitude_t altitude;
    mavlink_msg_altitude_decode(&message, &altitude);

    // Data from ALTITUDE message takes precedence over gps messages
    _altitudeMessageAvailable = true;
    _altitudeRelativeFact.setRawValue(altitude.altitude_relative);
    _altitudeAMSLFact.setRawValue(altitude.altitude_amsl);
}

void Vehicle::_setCapabilities(uint64_t capabilityBits)
{
    _capabilityBits = capabilityBits;
    _capabilityBitsKnown = true;
    emit capabilitiesKnownChanged(true);
    emit capabilityBitsChanged(_capabilityBits);

    QString supports("supports");
    QString doesNotSupport("does not support");

    qCDebug(VehicleLog) << QString("Vehicle %1 Mavlink 2.0").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MAVLINK2 ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 MISSION_ITEM_INT").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_INT ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 MISSION_COMMAND_INT").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_COMMAND_INT ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 GeoFence").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_FENCE ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 RallyPoints").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_RALLY ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 Terrain").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_TERRAIN ? supports : doesNotSupport);

    _setMaxProtoVersionFromBothSources();
}

void Vehicle::_setMaxProtoVersion(unsigned version) {

    // Set only once or if we need to reduce the max version
    if (_maxProtoVersion == 0 || version < _maxProtoVersion) {
        qCDebug(VehicleLog) << "_setMaxProtoVersion before:after" << _maxProtoVersion << version;
        _maxProtoVersion = version;
        emit requestProtocolVersion(_maxProtoVersion);
    }
}

void Vehicle::_setMaxProtoVersionFromBothSources()
{
    if (_mavlinkProtocolRequestComplete && _capabilityBitsKnown) {
        if (_mavlinkProtocolRequestMaxProtoVersion != 0) {
            qCDebug(VehicleLog) << "_setMaxProtoVersionFromBothSources using protocol version message";
            _setMaxProtoVersion(_mavlinkProtocolRequestMaxProtoVersion);
        } else {
            qCDebug(VehicleLog) << "_setMaxProtoVersionFromBothSources using capability bits";
            _setMaxProtoVersion(capabilityBits() & MAV_PROTOCOL_CAPABILITY_MAVLINK2 ? 200 : 100);
        }
    }
}

QString Vehicle::vehicleUIDStr()
{
    QString uid;
    uint8_t* pUid = (uint8_t*)(void*)&_uid;
    uid.asprintf("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
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

bool Vehicle::_apmArmingNotRequired()
{
    QString armingRequireParam("ARMING_REQUIRE");
    return _parameterManager->parameterExists(FactSystem::defaultComponentId, armingRequireParam) &&
            _parameterManager->getParameter(FactSystem::defaultComponentId, armingRequireParam)->rawValue().toInt() == 0;
}

void Vehicle::_handleSysStatus(mavlink_message_t& message)
{
    mavlink_sys_status_t sysStatus;
    mavlink_msg_sys_status_decode(&message, &sysStatus);

    _sysStatusSensorInfo.update(sysStatus);

    if (sysStatus.onboard_control_sensors_enabled & MAV_SYS_STATUS_PREARM_CHECK) {
        if (!_readyToFlyAvailable) {
            _readyToFlyAvailable = true;
            emit readyToFlyAvailableChanged(true);
        }

        bool newReadyToFly = sysStatus.onboard_control_sensors_health & MAV_SYS_STATUS_PREARM_CHECK;
        if (newReadyToFly != _readyToFly) {
            _readyToFly = newReadyToFly;
            emit readyToFlyChanged(_readyToFly);
        }
    }

    bool newAllSensorsHealthy = (sysStatus.onboard_control_sensors_enabled & sysStatus.onboard_control_sensors_health) == sysStatus.onboard_control_sensors_enabled;
    if (newAllSensorsHealthy != _allSensorsHealthy) {
        _allSensorsHealthy = newAllSensorsHealthy;
        emit allSensorsHealthyChanged(_allSensorsHealthy);
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
        emit sensorsUnhealthyBitsChanged(_onboardControlSensorsUnhealthy);
    }
}

void Vehicle::_handleBatteryStatus(mavlink_message_t& message)
{
    mavlink_battery_status_t batteryStatus;
    mavlink_msg_battery_status_decode(&message, &batteryStatus);

    if (!_lowestBatteryChargeStateAnnouncedMap.contains(batteryStatus.id)) {
        _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
    }

    QString batteryMessage;

    switch (batteryStatus.charge_state) {
    case MAV_BATTERY_CHARGE_STATE_OK:
        _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
        break;
    case MAV_BATTERY_CHARGE_STATE_LOW:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 level low");
        }
        break;
    case MAV_BATTERY_CHARGE_STATE_CRITICAL:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 level is critical");
        }
        break;
    case MAV_BATTERY_CHARGE_STATE_EMERGENCY:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 level emergency");
        }
        break;
    case MAV_BATTERY_CHARGE_STATE_FAILED:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 failed");
        }
        break;
    case MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 unhealthy");
        }
        break;
    }

    if (!batteryMessage.isEmpty()) {
        QString batteryIdStr("%1");
        if (_batteryFactGroupListModel.count() > 1) {
            batteryIdStr = batteryIdStr.arg(batteryStatus.id);
        } else {
            batteryIdStr = batteryIdStr.arg("");
        }
        _say(tr("warning"));
        _say(QStringLiteral("%1 %2 ").arg(_vehicleIdSpeech()).arg(batteryMessage.arg(batteryIdStr)));
    }
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
            _trajectoryPoints->start();
            _flightTimerStart();
            _clearCameraTriggerPoints();
            // Reset battery warning
            _lowestBatteryChargeStateAnnouncedMap.clear();
        } else {
            _trajectoryPoints->stop();
            _flightTimerStop();
            // Also handle Video Streaming
            if(qgcApp()->toolbox()->videoManager()->videoReceiver()) {
                if(_settingsManager->videoSettings()->disableWhenDisarmed()->rawValue().toBool()) {
                    _settingsManager->videoSettings()->streamEnabled()->setRawValue(false);
                    qgcApp()->toolbox()->videoManager()->videoReceiver()->stop();
                }
            }
        }
    }
}

void Vehicle::_handlePing(LinkInterface* link, mavlink_message_t& message)
{
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();

        mavlink_ping_t      ping;
        mavlink_message_t   msg;

        mavlink_msg_ping_decode(&message, &ping);
        mavlink_msg_ping_pack_chan(static_cast<uint8_t>(_mavlink->getSystemId()),
                                   static_cast<uint8_t>(_mavlink->getComponentId()),
                                   sharedLink->mavlinkChannel(),
                                   &msg,
                                   ping.time_usec,
                                   ping.seq,
                                   message.sysid,
                                   message.compid);
        sendMessageOnLinkThreadSafe(link, msg);
    }
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
        _base_mode   = heartbeat.base_mode;
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

// Pop warnings ignoring for mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#else
#pragma warning(pop, 0)
#endif

bool Vehicle::sendMessageOnLinkThreadSafe(LinkInterface* link, mavlink_message_t message)
{
    if (!link->isConnected()) {
        return false;
    }

    // Give the plugin a chance to adjust
    _firmwarePlugin->adjustOutgoingMavlinkMessageThreadSafe(this, link, &message);

    // Write message into buffer, prepending start sign
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buffer, &message);

    link->writeBytesThreadSafe((const char*)buffer, len);
    _messagesSent++;
    emit messagesSentChanged();

    return true;
}

int Vehicle::motorCount()
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
    case MAV_TYPE_SUBMARINE:
    {
        // Supported frame types
        enum {
            SUB_FRAME_BLUEROV1,
            SUB_FRAME_VECTORED,
            SUB_FRAME_VECTORED_6DOF,
            SUB_FRAME_VECTORED_6DOF_90DEG,
            SUB_FRAME_SIMPLEROV_3,
            SUB_FRAME_SIMPLEROV_4,
            SUB_FRAME_SIMPLEROV_5,
            SUB_FRAME_CUSTOM
        };

        uint8_t frameType = parameterManager()->getParameter(_compID, "FRAME_CONFIG")->rawValue().toInt();

        switch (frameType) {  // ardupilot/libraries/AP_Motors/AP_Motors6DOF.h sub_frame_t

        case SUB_FRAME_BLUEROV1:
        case SUB_FRAME_VECTORED:
            return 6;

        case SUB_FRAME_SIMPLEROV_3:
            return 3;

        case SUB_FRAME_SIMPLEROV_4:
            return 4;

        case SUB_FRAME_SIMPLEROV_5:
            return 5;

        case SUB_FRAME_VECTORED_6DOF:
        case SUB_FRAME_VECTORED_6DOF_90DEG:
        case SUB_FRAME_CUSTOM:
            return 8;

        default:
            return -1;
        }
    }

    default:
        return -1;
    }
}

bool Vehicle::coaxialMotors()
{
    return _firmwarePlugin->multiRotorCoaxialMotors(this);
}

bool Vehicle::xConfigMotors()
{
    return _firmwarePlugin->multiRotorXConfig(this);
}

QString Vehicle::formattedMessages()
{
    QString messages;
    for(UASMessage* message: _toolbox->uasMessageHandler()->messages()) {
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
    if (message) {
        emit newFormattedMessage(message->getFormatedText());
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

void Vehicle::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(QString(_settingsGroup).arg(_id));
    // Joystick enabled is a global setting so first make sure there are any joysticks connected
    if (_toolbox->joystickManager()->joysticks().count()) {
        setJoystickEnabled(settings.value(_joystickEnabledSettingsKey, false).toBool());
        _startJoystick(true);
    }
}

void Vehicle::_saveSettings()
{
    QSettings settings;
    settings.beginGroup(QString(_settingsGroup).arg(_id));

    // The joystick enabled setting should only be changed if a joystick is present
    // since the checkbox can only be clicked if one is present
    if (_toolbox->joystickManager()->joysticks().count()) {
        settings.setValue(_joystickEnabledSettingsKey, _joystickEnabled);
    }
}

bool Vehicle::joystickEnabled()
{
    return _joystickEnabled;
}

void Vehicle::setJoystickEnabled(bool enabled)
{
    _joystickEnabled = enabled;
    _startJoystick(_joystickEnabled);
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

QGeoCoordinate Vehicle::homePosition()
{
    return _homePosition;
}

void Vehicle::setArmed(bool armed, bool showError)
{
    // We specifically use COMMAND_LONG:MAV_CMD_COMPONENT_ARM_DISARM since it is supported by more flight stacks.
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_COMPONENT_ARM_DISARM,
                   showError,
                   armed ? 1.0f : 0.0f);
}

void Vehicle::forceArm(void)
{
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_COMPONENT_ARM_DISARM,
                   true,    // show error if fails
                   1.0f,    // arm
                   2989);   // force arm
}

bool Vehicle::flightModeSetAvailable()
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::SetFlightModeCapability);
}

QStringList Vehicle::flightModes()
{
    return _firmwarePlugin->flightModes(this);
}

QStringList Vehicle::extraJoystickFlightModes()
{
    return _firmwarePlugin->extraJoystickFlightModes(this);
}

QString Vehicle::flightMode() const
{
    return _firmwarePlugin->flightMode(_base_mode, _custom_mode);
}

void Vehicle::setFlightMode(const QString& flightMode)
{
    uint8_t     base_mode;
    uint32_t    custom_mode;

    if (_firmwarePlugin->setFlightMode(flightMode, &base_mode, &custom_mode)) {
        WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

        if (!weakLink.expired()) {
            uint8_t                 newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_CUSTOM_MODE;
            SharedLinkInterfacePtr  sharedLink = weakLink.lock();

            // setFlightMode will only set MAV_MODE_FLAG_CUSTOM_MODE_ENABLED in base_mode, we need to move back in the existing
            // states.
            newBaseMode |= base_mode;

            mavlink_message_t msg;
            mavlink_msg_set_mode_pack_chan(_mavlink->getSystemId(),
                                           _mavlink->getComponentId(),
                                           sharedLink->mavlinkChannel(),
                                           &msg,
                                           id(),
                                           newBaseMode,
                                           custom_mode);
            sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }
    } else {
        qWarning() << "FirmwarePlugin::setFlightMode failed, flightMode:" << flightMode;
    }
}

#if 0
QVariantList Vehicle::links() const {
    QVariantList ret;

    for( const auto &item: _links )
        ret << QVariant::fromValue(item);

    return ret;
}
#endif

void Vehicle::requestDataStream(MAV_DATA_STREAM stream, uint16_t rate, bool sendMultiple)
{
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        mavlink_message_t               msg;
        mavlink_request_data_stream_t   dataStream;
        SharedLinkInterfacePtr          sharedLink = weakLink.lock();

        memset(&dataStream, 0, sizeof(dataStream));

        dataStream.req_stream_id = stream;
        dataStream.req_message_rate = rate;
        dataStream.start_stop = 1;  // start
        dataStream.target_system = id();
        dataStream.target_component = _defaultComponentId;

        mavlink_msg_request_data_stream_encode_chan(_mavlink->getSystemId(),
                                                    _mavlink->getComponentId(),
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,
                                                    &dataStream);

        if (sendMultiple) {
            // We use sendMessageMultiple since we really want these to make it to the vehicle
            sendMessageMultiple(msg);
        } else {
            sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }
    }
}

void Vehicle::_sendMessageMultipleNext()
{
    if (_nextSendMessageMultipleIndex < _sendMessageMultipleList.count()) {
        qCDebug(VehicleLog) << "_sendMessageMultipleNext:" << _sendMessageMultipleList[_nextSendMessageMultipleIndex].message.msgid;

        WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

        if (!weakLink.expired()) {
            sendMessageOnLinkThreadSafe(weakLink.lock().get(), _sendMessageMultipleList[_nextSendMessageMultipleIndex].message);
        }

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
    qgcApp()->showAppMessage(tr("Mission transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_geoFenceManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showAppMessage(tr("GeoFence transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_rallyPointManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showAppMessage(tr("Rally Point transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_clearCameraTriggerPoints()
{
    _cameraTriggerPoints.clearAndDeleteContents();
}

void Vehicle::_flightTimerStart()
{
    _flightTimer.start();
    _flightTimeUpdater.start();
    _flightDistanceFact.setRawValue(0);
    _flightTimeFact.setRawValue(0);
}

void Vehicle::_flightTimerStop()
{
    _flightTimeUpdater.stop();
}

void Vehicle::_updateFlightTime()
{
    _flightTimeFact.setRawValue((double)_flightTimer.elapsed() / 1000.0);
}

void Vehicle::_firstMissionLoadComplete()
{
    disconnect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_firstMissionLoadComplete);
    _initialConnectStateMachine->advance();
}

void Vehicle::_firstGeoFenceLoadComplete()
{
    disconnect(_geoFenceManager, &GeoFenceManager::loadComplete, this, &Vehicle::_firstGeoFenceLoadComplete);
    _initialConnectStateMachine->advance();
}

void Vehicle::_firstRallyPointLoadComplete()
{
    disconnect(_rallyPointManager, &RallyPointManager::loadComplete, this, &Vehicle::_firstRallyPointLoadComplete);
    _initialPlanRequestComplete = true;
    emit initialPlanRequestCompleteChanged(true);
    _initialConnectStateMachine->advance();
}

void Vehicle::_parametersReady(bool parametersReady)
{
    qDebug() << "_parametersReady" << parametersReady;

    // Try to set current unix time to the vehicle
    _sendQGCTimeToVehicle();
    // Send time twice, more likely to get to the vehicle on a noisy link
    _sendQGCTimeToVehicle();
    if (parametersReady) {
        disconnect(_parameterManager, &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);
        _setupAutoDisarmSignalling();
        _initialConnectStateMachine->advance();
    }
}

void Vehicle::_sendQGCTimeToVehicle()
{
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        mavlink_message_t       msg;
        mavlink_system_time_t   cmd;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        // Timestamp of the master clock in microseconds since UNIX epoch.
        cmd.time_unix_usec = QDateTime::currentDateTime().currentMSecsSinceEpoch()*1000;
        // Timestamp of the component clock since boot time in milliseconds (Not necessary).
        cmd.time_boot_ms = 0;
        mavlink_msg_system_time_encode_chan(_mavlink->getSystemId(),
                                            _mavlink->getComponentId(),
                                            sharedLink->mavlinkChannel(),
                                            &msg,
                                            &cmd);

        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
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
    if (!_joystickEnabled) {
        sendJoystickDataThreadSafe(
                    static_cast<float>(roll),
                    static_cast<float>(pitch),
                    static_cast<float>(yaw),
                    static_cast<float>(thrust),
                    0);
    }
}

void Vehicle::_say(const QString& text)
{
    _toolbox->audioOutput()->say(text.toLower());
}

bool Vehicle::airship() const
{
    return QGCMAVLink::isAirship(vehicleType());
}

bool Vehicle::fixedWing() const
{
    return QGCMAVLink::isFixedWing(vehicleType());
}

bool Vehicle::rover() const
{
    return QGCMAVLink::isRoverBoat(vehicleType());
}

bool Vehicle::sub() const
{
    return QGCMAVLink::isSub(vehicleType());
}

bool Vehicle::multiRotor() const
{
    return QGCMAVLink::isMultiRotor(vehicleType());
}

bool Vehicle::vtol() const
{
    return QGCMAVLink::isVTOL(vehicleType());
}

bool Vehicle::supportsThrottleModeCenterZero() const
{
    return _firmwarePlugin->supportsThrottleModeCenterZero();
}

bool Vehicle::supportsNegativeThrust()
{
    return _firmwarePlugin->supportsNegativeThrust(this);
}

bool Vehicle::supportsRadio() const
{
    return _firmwarePlugin->supportsRadio();
}

bool Vehicle::supportsJSButton() const
{
    return _firmwarePlugin->supportsJSButton();
}

bool Vehicle::supportsMotorInterference() const
{
    return _firmwarePlugin->supportsMotorInterference();
}

bool Vehicle::supportsTerrainFrame() const
{
    return !px4Firmware();
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
QString Vehicle::_vehicleIdSpeech()
{
    if (_toolbox->multiVehicleManager()->vehicles()->count() > 1) {
        return tr("Vehicle %1 ").arg(id());
    } else {
        return QString();
    }
}

void Vehicle::_handleFlightModeChanged(const QString& flightMode)
{
    _say(tr("%1 %2 flight mode").arg(_vehicleIdSpeech()).arg(flightMode));
    emit guidedModeChanged(_firmwarePlugin->isGuidedMode(this));
}

void Vehicle::_announceArmedChanged(bool armed)
{
    _say(QString("%1 %2").arg(_vehicleIdSpeech()).arg(armed ? tr("armed") : tr("disarmed")));
    if(armed) {
        //-- Keep track of armed coordinates
        _armedPosition = _coordinate;
        emit armedPositionChanged();
    }
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

bool Vehicle::guidedModeSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::GuidedModeCapability);
}

bool Vehicle::pauseVehicleSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::PauseVehicleCapability);
}

bool Vehicle::orbitModeSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::OrbitModeCapability);
}

bool Vehicle::roiModeSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::ROIModeCapability);
}

bool Vehicle::takeoffVehicleSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::TakeoffVehicleCapability);
}

QString Vehicle::gotoFlightMode() const
{
    return _firmwarePlugin->gotoFlightMode();
}

void Vehicle::guidedModeRTL(bool smartRTL)
{
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeRTL(this, smartRTL);
}

void Vehicle::guidedModeLand()
{
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeLand(this);
}

void Vehicle::guidedModeTakeoff(double altitudeRelative)
{
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeTakeoff(this, altitudeRelative);
}

double Vehicle::minimumTakeoffAltitude()
{
    return _firmwarePlugin->minimumTakeoffAltitude(this);
}

void Vehicle::startMission()
{
    _firmwarePlugin->startMission(this);
}

void Vehicle::guidedModeGotoLocation(const QGeoCoordinate& gotoCoord)
{
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    if (!coordinate().isValid()) {
        return;
    }
    double maxDistance = _settingsManager->flyViewSettings()->maxGoToLocationDistance()->rawValue().toDouble();
    if (coordinate().distanceTo(gotoCoord) > maxDistance) {
        qgcApp()->showAppMessage(QString("New location is too far. Must be less than %1 %2.").arg(qRound(FactMetaData::metersToAppSettingsHorizontalDistanceUnits(maxDistance).toDouble())).arg(FactMetaData::appSettingsHorizontalDistanceUnitsString()));
        return;
    }
    _firmwarePlugin->guidedModeGotoLocation(this, gotoCoord);
}

void Vehicle::guidedModeChangeAltitude(double altitudeChange)
{
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeAltitude(this, altitudeChange);
}

void Vehicle::guidedModeOrbit(const QGeoCoordinate& centerCoord, double radius, double amslAltitude)
{
    if (!orbitModeSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("Orbit mode not supported by Vehicle."));
        return;
    }
    if (capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        sendMavCommandInt(
                    defaultComponentId(),
                    MAV_CMD_DO_ORBIT,
                    MAV_FRAME_GLOBAL,
                    true,                           // show error if fails
                    static_cast<float>(radius),
                    static_cast<float>(qQNaN()),    // Use default velocity
                    0,                              // Vehicle points to center
                    static_cast<float>(qQNaN()),    // reserved
                    centerCoord.latitude(), centerCoord.longitude(), static_cast<float>(amslAltitude));
    } else {
        sendMavCommand(
                    defaultComponentId(),
                    MAV_CMD_DO_ORBIT,
                    true,                           // show error if fails
                    static_cast<float>(radius),
                    static_cast<float>(qQNaN()),    // Use default velocity
                    0,                              // Vehicle points to center
                    static_cast<float>(qQNaN()),    // reserved
                    static_cast<float>(centerCoord.latitude()),
                    static_cast<float>(centerCoord.longitude()),
                    static_cast<float>(amslAltitude));
    }
}

void Vehicle::guidedModeROI(const QGeoCoordinate& centerCoord)
{
    if (!roiModeSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("ROI mode not supported by Vehicle."));
        return;
    }
    if (capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        sendMavCommandInt(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_LOCATION,
                    MAV_FRAME_GLOBAL,
                    true,                           // show error if fails
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    centerCoord.latitude(),
                    centerCoord.longitude(),
                    static_cast<float>(centerCoord.altitude()));
    } else {
        sendMavCommand(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_LOCATION,
                    true,                           // show error if fails
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(centerCoord.latitude()),
                    static_cast<float>(centerCoord.longitude()),
                    static_cast<float>(centerCoord.altitude()));
    }
}

void Vehicle::stopGuidedModeROI()
{
    if (!roiModeSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("ROI mode not supported by Vehicle."));
        return;
    }
    if (capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        sendMavCommandInt(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_NONE,
                    MAV_FRAME_GLOBAL,
                    true,                           // show error if fails
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<double>(qQNaN()),   // Empty
                    static_cast<double>(qQNaN()),   // Empty
                    static_cast<float>(qQNaN()));   // Empty
    } else {
        sendMavCommand(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_NONE,
                    true,                           // show error if fails
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()));   // Empty
    }
}

void Vehicle::pauseVehicle()
{
    if (!pauseVehicleSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("Pause not supported by vehicle."));
        return;
    }
    _firmwarePlugin->pauseVehicle(this);
}

void Vehicle::abortLanding(double climbOutAltitude)
{
    sendMavCommand(
                defaultComponentId(),
                MAV_CMD_DO_GO_AROUND,
                true,        // show error if fails
                static_cast<float>(climbOutAltitude));
}

bool Vehicle::guidedMode() const
{
    return _firmwarePlugin->isGuidedMode(this);
}

void Vehicle::setGuidedMode(bool guidedMode)
{
    return _firmwarePlugin->setGuidedMode(this, guidedMode);
}

void Vehicle::emergencyStop()
{
    sendMavCommand(
                _defaultComponentId,
                MAV_CMD_COMPONENT_ARM_DISARM,
                true,        // show error if fails
                0.0f,
                21196.0f);  // Magic number for emergency stop
}

void Vehicle::setCurrentMissionSequence(int seq)
{
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        mavlink_message_t       msg;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        if (!_firmwarePlugin->sendHomePositionToVehicle()) {
            seq--;
        }
        mavlink_msg_mission_set_current_pack_chan(
                    static_cast<uint8_t>(_mavlink->getSystemId()),
                    static_cast<uint8_t>(_mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    static_cast<uint8_t>(id()),
                    _compID,
                    static_cast<uint16_t>(seq));
        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void Vehicle::sendMavCommand(int compId, MAV_CMD command, bool showError, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _sendMavCommandWorker(false,            // commandInt
                          false,            // requestMessage
                          showError,
                          nullptr,          // resultHandler
                          nullptr,          // resultHandlerData
                          compId,
                          command,
                          MAV_FRAME_GLOBAL,
                          param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendCommand(int compId, int command, bool showError, double param1, double param2, double param3, double param4, double param5, double param6, double param7)
{
    sendMavCommand(
        compId, static_cast<MAV_CMD>(command),
        showError,
        static_cast<float>(param1),
        static_cast<float>(param2),
        static_cast<float>(param3),
        static_cast<float>(param4),
        static_cast<float>(param5),
        static_cast<float>(param6),
        static_cast<float>(param7));
}

void Vehicle::sendMavCommandWithHandler(MavCmdResultHandler resultHandler, void *resultHandlerData, int compId, MAV_CMD command, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _sendMavCommandWorker(false,                // commandInt
                          false,                // requestMessage,
                          false,                // showError
                          resultHandler,
                          resultHandlerData,
                          compId,
                          command,
                          MAV_FRAME_GLOBAL,
                          param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandInt(int compId, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    _sendMavCommandWorker(true,         // commandInt
                          false,        // requestMessage
                          showError,
                          nullptr,      // resultHandler
                          nullptr,      // resultHandlerData
                          compId,
                          command,
                          frame,
                          param1, param2, param3, param4, param5, param6, param7);
}

int Vehicle::_findMavCommandListEntryIndex(int targetCompId, MAV_CMD command)
{
    for (int i=0; i<_mavCommandList.count(); i++) {
        const MavCommandListEntry_t& entry = _mavCommandList[i];
        if (entry.targetCompId == targetCompId && entry.command == command) {
            return i;
        }
    }

    return -1;
}

bool Vehicle::_sendMavCommandShouldRetry(MAV_CMD command)
{
    switch (command) {
#ifdef QT_DEBUG
    // These MockLink command should be retried so we can create unit tests to test retry code
    case MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED:
    case MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED:
    case MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED:
    case MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED:
    case MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE:
        return true;
#endif

    // In general we should not retry any commands. This is for safety reasons. For example you don't want an ARM command
    // to timeout with no response over a noisy link twice and then suddenly have the third try work 6 seconds later. At that
    // point the user could have walked up to the vehicle to see what is going wrong.
    //
    // We do retry commands which are part of the initial vehicle connect sequence. This makes this process work better over noisy
    // links where commands could be lost. Also these commands tend to just be requesting status so if they end up being delayed
    // there are no safety concerns that could occur.
    case MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES:
    case MAV_CMD_REQUEST_PROTOCOL_VERSION:
    case MAV_CMD_REQUEST_MESSAGE:
    case MAV_CMD_PREFLIGHT_STORAGE:
        return true;

    default:
        return false;
    }
}

void Vehicle::_sendMavCommandWorker(bool commandInt, bool requestMessage, bool showError, MavCmdResultHandler resultHandler, void* resultHandlerData, int targetCompId, MAV_CMD command, MAV_FRAME frame, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    int entryIndex = _findMavCommandListEntryIndex(targetCompId, command);
    if (entryIndex != -1 || targetCompId == MAV_COMP_ID_ALL) {
        bool    compIdAll       = targetCompId == MAV_COMP_ID_ALL;
        QString rawCommandName  = _toolbox->missionCommandTree()->rawName(command);

        qCDebug(VehicleLog) << QStringLiteral("_sendMavCommandWorker failing %1").arg(compIdAll ? "MAV_COMP_ID_ALL not supportded" : "duplicate command") << rawCommandName;

        // If we send multiple versions of the same command to a component there is no way to discern which COMMAND_ACK we get back goes with which.
        // Because of this we fail in that case.
        MavCmdResultFailureCode_t failureCode = compIdAll ? MavCmdResultCommandResultOnly : MavCmdResultFailureDuplicateCommand;
        if (resultHandler) {
            (*resultHandler)(resultHandlerData, targetCompId, MAV_RESULT_FAILED, failureCode);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (showError) {
            qgcApp()->showAppMessage(tr("Unable to send command: %1.").arg(compIdAll ? tr("Internal error - MAV_COMP_ID_ALL not supported") : tr("Waiting on previous response to same command.")));
        }

        return;
    }

    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        MavCommandListEntry_t   entry;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        entry.useCommandInt     = commandInt;
        entry.targetCompId      = targetCompId;
        entry.command           = command;
        entry.frame             = frame;
        entry.showError         = showError;
        entry.requestMessage    = requestMessage;
        entry.resultHandler     = resultHandler;
        entry.resultHandlerData = resultHandlerData;
        entry.rgParam[0]        = param1;
        entry.rgParam[1]        = param2;
        entry.rgParam[2]        = param3;
        entry.rgParam[3]        = param4;
        entry.rgParam[4]        = param5;
        entry.rgParam[5]        = param6;
        entry.rgParam[6]        = param7;
        entry.maxTries          = _sendMavCommandShouldRetry(command) ? _mavCommandMaxRetryCount : 1;
        entry.ackTimeoutMSecs   = sharedLink->linkConfiguration()->isHighLatency() ? _mavCommandAckTimeoutMSecsHighLatency : _mavCommandAckTimeoutMSecs;
        entry.elapsedTimer.start();

        _mavCommandList.append(entry);
        _sendMavCommandFromList(_mavCommandList.last());
    }
}

void Vehicle::_sendMavCommandFromList(MavCommandListEntry_t& commandEntry)
{
    QString rawCommandName  = _toolbox->missionCommandTree()->rawName(commandEntry.command);

    if (++commandEntry.tryCount > commandEntry.maxTries) {
        qCDebug(VehicleLog) << "_sendMavCommandFromList giving up after max retries" << rawCommandName;
        if (commandEntry.resultHandler) {
            (*commandEntry.resultHandler)(commandEntry.resultHandlerData, commandEntry.targetCompId, MAV_RESULT_FAILED, MavCmdResultFailureNoResponseToCommand);
        } else {
            emit mavCommandResult(_id, commandEntry.targetCompId, commandEntry.command, MAV_RESULT_FAILED, MavCmdResultFailureNoResponseToCommand);
        }
        if (commandEntry.showError) {
            qgcApp()->showAppMessage(tr("Vehicle did not respond to command: %1").arg(rawCommandName));
        }
        _mavCommandList.removeAt(_findMavCommandListEntryIndex(commandEntry.targetCompId, commandEntry.command));
        return;
    }

    if (commandEntry.tryCount > 1 && !px4Firmware() && commandEntry.command == MAV_CMD_START_RX_PAIR) {
        // The implementation of this command comes from the IO layer and is shared across stacks. So for other firmwares
        // we aren't really sure whether they are correct or not.
        return;
    }

    if (commandEntry.requestMessage) {
        RequestMessageInfo_t* pInfo = static_cast<RequestMessageInfo_t*>(commandEntry.resultHandlerData);
        _waitForMavlinkMessage(_requestMessageWaitForMessageResultHandler, pInfo, pInfo->msgId, 1000);
    }

    qCDebug(VehicleLog) << "_sendMavCommandFromList command:tryCount" << rawCommandName << commandEntry.tryCount;

    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        mavlink_message_t       msg;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        if (commandEntry.useCommandInt) {
            mavlink_command_int_t  cmd;

            memset(&cmd, 0, sizeof(cmd));
            cmd.target_system =     _id;
            cmd.target_component =  commandEntry.targetCompId;
            cmd.command =           commandEntry.command;
            cmd.frame =             commandEntry.frame;
            cmd.param1 =            commandEntry.rgParam[0];
            cmd.param2 =            commandEntry.rgParam[1];
            cmd.param3 =            commandEntry.rgParam[2];
            cmd.param4 =            commandEntry.rgParam[3];
            cmd.x =                 commandEntry.frame == MAV_FRAME_MISSION ? commandEntry.rgParam[4] : commandEntry.rgParam[4] * 1e7;
            cmd.y =                 commandEntry.frame == MAV_FRAME_MISSION ? commandEntry.rgParam[5] : commandEntry.rgParam[5] * 1e7;
            cmd.z =                 commandEntry.rgParam[6];
            mavlink_msg_command_int_encode_chan(_mavlink->getSystemId(),
                                                _mavlink->getComponentId(),
                                                sharedLink->mavlinkChannel(),
                                                &msg,
                                                &cmd);
        } else {
            mavlink_command_long_t  cmd;

            memset(&cmd, 0, sizeof(cmd));
            cmd.target_system =     _id;
            cmd.target_component =  commandEntry.targetCompId;
            cmd.command =           commandEntry.command;
            cmd.confirmation =      0;
            cmd.param1 =            commandEntry.rgParam[0];
            cmd.param2 =            commandEntry.rgParam[1];
            cmd.param3 =            commandEntry.rgParam[2];
            cmd.param4 =            commandEntry.rgParam[3];
            cmd.param5 =            commandEntry.rgParam[4];
            cmd.param6 =            commandEntry.rgParam[5];
            cmd.param7 =            commandEntry.rgParam[6];
            mavlink_msg_command_long_encode_chan(_mavlink->getSystemId(),
                                                 _mavlink->getComponentId(),
                                                 sharedLink->mavlinkChannel(),
                                                 &msg,
                                                 &cmd);
        }

        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void Vehicle::_sendMavCommandResponseTimeoutCheck(void)
{
    if (_mavCommandList.isEmpty()) {
        return;
    }

    // Walk the list backwards since _sendMavCommandFromList can remove entries
    for (int i=_mavCommandList.count()-1; i>=0; i--) {
        MavCommandListEntry_t& commandEntry = _mavCommandList[i];
        if (commandEntry.elapsedTimer.elapsed() > commandEntry.ackTimeoutMSecs) {
            // Try sending command again
            _sendMavCommandFromList(commandEntry);
        }
    }
}

void Vehicle::_handleCommandAck(mavlink_message_t& message)
{
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&message, &ack);

    QString rawCommandName  =_toolbox->missionCommandTree()->rawName(static_cast<MAV_CMD>(ack.command));
    qCDebug(VehicleLog) << QStringLiteral("_handleCommandAck command(%1) result(%2)").arg(rawCommandName).arg(QGCMAVLink::mavResultToString(static_cast<MAV_RESULT>(ack.result)));

    if (ack.command == MAV_CMD_DO_SET_ROI_LOCATION) {
        if (ack.result == MAV_RESULT_ACCEPTED) {
            _isROIEnabled = true;
            emit isROIEnabledChanged();
        }
    }

    if (ack.command == MAV_CMD_DO_SET_ROI_NONE) {
        if (ack.result == MAV_RESULT_ACCEPTED) {
            _isROIEnabled = false;
            emit isROIEnabledChanged();
        }
    }

#if !defined(NO_ARDUPILOT_DIALECT)
    if (ack.command == MAV_CMD_FLASH_BOOTLOADER && ack.result == MAV_RESULT_ACCEPTED) {
        qgcApp()->showAppMessage(tr("Bootloader flash succeeded"));
    }
#endif

    int entryIndex = _findMavCommandListEntryIndex(message.compid, static_cast<MAV_CMD>(ack.command));
    bool commandInList = false;
    if (entryIndex != -1) {
        const MavCommandListEntry_t& commandEntry = _mavCommandList[entryIndex];
        if (commandEntry.command == ack.command) {
            if (commandEntry.requestMessage) {
                RequestMessageInfo_t* pInfo = static_cast<RequestMessageInfo_t*>(commandEntry.resultHandlerData);
                pInfo->commandAckReceived = true;
                if (ack.result == MAV_RESULT_ACCEPTED) {
                    if (pInfo->messageReceived) {
                        delete pInfo;
                    } else {
                        _waitForMavlinkMessageTimeoutActive = true;
                        _waitForMavlinkMessageElapsed.restart();
                    }
                } else {
                    if (pInfo->messageReceived) {
                        qCWarning(VehicleLog) << "Internal Error: _handleCommandAck for requestMessage with result failure, but message already received";
                    } else {
                        _waitForMavlinkMessageClear();
                        (*commandEntry.resultHandler)(commandEntry.resultHandlerData, message.compid, static_cast<MAV_RESULT>(ack.result), MavCmdResultCommandResultOnly);
                    }
                }
            } else {
                if (commandEntry.resultHandler) {
                    (*commandEntry.resultHandler)(commandEntry.resultHandlerData, message.compid, static_cast<MAV_RESULT>(ack.result), MavCmdResultCommandResultOnly);
                } else {
                    if (commandEntry.showError) {
                        switch (ack.result) {
                        case MAV_RESULT_TEMPORARILY_REJECTED:
                            qgcApp()->showAppMessage(tr("%1 command temporarily rejected").arg(rawCommandName));
                            break;
                        case MAV_RESULT_DENIED:
                            qgcApp()->showAppMessage(tr("%1 command denied").arg(rawCommandName));
                            break;
                        case MAV_RESULT_UNSUPPORTED:
                            qgcApp()->showAppMessage(tr("%1 command not supported").arg(rawCommandName));
                            break;
                        case MAV_RESULT_FAILED:
                            qgcApp()->showAppMessage(tr("%1 command failed").arg(rawCommandName));
                            break;
                        default:
                            // Do nothing
                            break;
                        }
                    }
                    emit mavCommandResult(_id, message.compid, ack.command, ack.result, MavCmdResultCommandResultOnly);
                }
            }

            _mavCommandList.removeAt(entryIndex);
            commandInList = true;
        }
    }

    if (!commandInList) {
        qCDebug(VehicleLog) << "_handleCommandAck Ack not in list" << rawCommandName;
    }

    // advance PID tuning setup/teardown
    if (ack.command == MAV_CMD_SET_MESSAGE_INTERVAL && _pidTuningNextAdjustIndex >= 0) {
        _pidTuningAdjustRates();
    }
    if (ack.command == MAV_CMD_GET_MESSAGE_INTERVAL && _pidTuningWaitingForRates) {
        if (_pidTuningMessageRatesUsecs.count() < _pidTuningMessages.count()) {
            sendMavCommand(defaultComponentId(),
                    MAV_CMD_GET_MESSAGE_INTERVAL,
                    true,                        // show error
                    _pidTuningMessages[_pidTuningMessageRatesUsecs.count()]);
        }
    }
}

void Vehicle::_waitForMavlinkMessage(WaitForMavlinkMessageResultHandler resultHandler, void* resultHandlerData, int messageId, int timeoutMsecs)
{
    qCDebug(VehicleLog) << "_waitForMavlinkMessage msg:timeout" << messageId << timeoutMsecs;
    _waitForMavlinkMessageResultHandler     = resultHandler;
    _waitForMavlinkMessageResultHandlerData = resultHandlerData;
    _waitForMavlinkMessageId                = messageId;
    _waitForMavlinkMessageTimeoutActive     = false;
    _waitForMavlinkMessageTimeoutMsecs      = timeoutMsecs;
}

void Vehicle::_waitForMavlinkMessageClear(void)
{
    qCDebug(VehicleLog) << "_waitForMavlinkMessageClear";
    _waitForMavlinkMessageResultHandler     = nullptr;
    _waitForMavlinkMessageResultHandlerData = nullptr;
    _waitForMavlinkMessageId                = 0;
}

void Vehicle::_waitForMavlinkMessageMessageReceived(const mavlink_message_t& message)
{
    if (_waitForMavlinkMessageId != 0) {
        if (_waitForMavlinkMessageId == message.msgid) {
            WaitForMavlinkMessageResultHandler  resultHandler       = _waitForMavlinkMessageResultHandler;
            void*                               resultHandlerData   = _waitForMavlinkMessageResultHandlerData;
            qCDebug(VehicleLog) << "_waitForMavlinkMessageMessageReceived message received" << _waitForMavlinkMessageId;
            _waitForMavlinkMessageClear();
            (*resultHandler)(resultHandlerData, false /* noResponseFromVehicle */, message);
        } else if (_waitForMavlinkMessageTimeoutActive && _waitForMavlinkMessageElapsed.elapsed() > _waitForMavlinkMessageTimeoutMsecs) {
            WaitForMavlinkMessageResultHandler  resultHandler       = _waitForMavlinkMessageResultHandler;
            void*                               resultHandlerData   = _waitForMavlinkMessageResultHandlerData;
            qCDebug(VehicleLog) << "_waitForMavlinkMessageMessageReceived message timed out" << _waitForMavlinkMessageId;
            _waitForMavlinkMessageClear();
            (*resultHandler)(resultHandlerData, true /* noResponseFromVehicle */, message);
        }
    }
}

void Vehicle::requestMessage(RequestMessageResultHandler resultHandler, void* resultHandlerData, int compId, int messageId, float param1, float param2, float param3, float param4, float param5)
{
    RequestMessageInfo_t* pInfo = new RequestMessageInfo_t;

    *pInfo                      = { };
    pInfo->msgId                = messageId;
    pInfo->compId               = compId;
    pInfo->resultHandler        = resultHandler;
    pInfo->resultHandlerData    = resultHandlerData;

    _sendMavCommandWorker(false,                                    // commandInt
                          true,                                     // requestMessage,
                          false,                                    // showError
                          _requestMessageCmdResultHandler,
                          pInfo,                                    // resultHandlerData
                          compId,
                          MAV_CMD_REQUEST_MESSAGE,
                          MAV_FRAME_GLOBAL,
                          messageId,
                          param1, param2, param3, param4, param5, 0);
}

void Vehicle::_requestMessageCmdResultHandler(void* resultHandlerData, int /*compId*/, MAV_RESULT result, MavCmdResultFailureCode_t failureCode)
{
    RequestMessageInfo_t* pInfo   = static_cast<RequestMessageInfo_t*>(resultHandlerData);

    pInfo->commandAckReceived = true;
    if (result != MAV_RESULT_ACCEPTED) {
        mavlink_message_t                           message;
        RequestMessageResultHandlerFailureCode_t    requestMessageFailureCode;

        switch (failureCode) {
        case Vehicle::MavCmdResultCommandResultOnly:
            requestMessageFailureCode = RequestMessageFailureCommandError;
            break;
        case Vehicle::MavCmdResultFailureNoResponseToCommand:
            requestMessageFailureCode = RequestMessageFailureCommandNotAcked;
            break;
        case Vehicle::MavCmdResultFailureDuplicateCommand:
            requestMessageFailureCode = RequestMessageFailureDuplicateCommand;
            break;
        }

        (*pInfo->resultHandler)(pInfo->resultHandlerData, result,  requestMessageFailureCode, message);
    }
    if (pInfo->messageReceived) {
        delete pInfo;
    }
}

void Vehicle::_requestMessageWaitForMessageResultHandler(void* resultHandlerData, bool noResponsefromVehicle, const mavlink_message_t& message)
{
    RequestMessageInfo_t* pInfo = static_cast<RequestMessageInfo_t*>(resultHandlerData);

    pInfo->messageReceived = true;
    (*pInfo->resultHandler)(pInfo->resultHandlerData, noResponsefromVehicle ? MAV_RESULT_FAILED : MAV_RESULT_ACCEPTED, noResponsefromVehicle ? RequestMessageFailureMessageNotReceived : RequestMessageNoFailure, message);
}

void Vehicle::setPrearmError(const QString& prearmError)
{
    _prearmError = prearmError;
    emit prearmErrorChanged(_prearmError);
    if (!_prearmError.isEmpty()) {
        _prearmErrorTimer.start();
    }
}

void Vehicle::_prearmErrorTimeout()
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

QString Vehicle::firmwareVersionTypeString() const
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

void Vehicle::_rebootCommandResultHandler(void* resultHandlerData, int /*compId*/, MAV_RESULT commandResult, MavCmdResultFailureCode_t failureCode)
{
    Vehicle* vehicle = static_cast<Vehicle*>(resultHandlerData);

    if (commandResult != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
        case MavCmdResultCommandResultOnly:
            qCDebug(VehicleLog) << QStringLiteral("MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN error(%1)").arg(commandResult);
            break;
        case MavCmdResultFailureNoResponseToCommand:
            qCDebug(VehicleLog) << "MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN failed: no response from vehicle";
            break;
        case MavCmdResultFailureDuplicateCommand:
            qCDebug(VehicleLog) << "MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN failed: duplicate command";
            break;
        }
        qgcApp()->showAppMessage(tr("Vehicle reboot failed."));
    } else {
        vehicle->closeVehicle();
    }
}

void Vehicle::rebootVehicle()
{
    sendMavCommandWithHandler(_rebootCommandResultHandler, this, _defaultComponentId, MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1);
}

void Vehicle::startCalibration(Vehicle::CalibrationType calType)
{
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();

        float param1 = 0;
        float param2 = 0;
        float param3 = 0;
        float param4 = 0;
        float param5 = 0;
        float param6 = 0;
        float param7 = 0;

        switch (calType) {
        case CalibrationGyro:
            param1 = 1;
            break;
        case CalibrationMag:
            param2 = 1;
            break;
        case CalibrationRadio:
            param4 = 1;
            break;
        case CalibrationCopyTrims:
            param4 = 2;
            break;
        case CalibrationAccel:
            param5 = 1;
            break;
        case CalibrationLevel:
            param5 = 2;
            break;
        case CalibrationEsc:
            param7 = 1;
            break;
        case CalibrationPX4Airspeed:
            param6 = 1;
            break;
        case CalibrationPX4Pressure:
            param3 = 1;
            break;
        case CalibrationAPMCompassMot:
            param6 = 1;
            break;
        case CalibrationAPMPressureAirspeed:
            param3 = 1;
            break;
        case CalibrationAPMPreFlight:
            param3 = 1; // GroundPressure/Airspeed
            if (multiRotor() || rover()) {
                // Gyro cal for ArduCopter only
                param1 = 1;
            }
        }

        // We can't use sendMavCommand here since we have no idea how long it will be before the command returns a result. This in turn
        // causes the retry logic to break down.
        mavlink_message_t msg;
        mavlink_msg_command_long_pack_chan(_mavlink->getSystemId(),
                                           _mavlink->getComponentId(),
                                           sharedLink->mavlinkChannel(),
                                           &msg,
                                           id(),
                                           defaultComponentId(),            // target component
                                           MAV_CMD_PREFLIGHT_CALIBRATION,    // command id
                                           0,                                // 0=first transmission of command
                                           param1, param2, param3, param4, param5, param6, param7);
        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void Vehicle::stopCalibration(void)
{
    sendMavCommand(defaultComponentId(),    // target component
                   MAV_CMD_PREFLIGHT_CALIBRATION,     // command id
                   true,                              // showError
                   0,                                 // gyro cal
                   0,                                 // mag cal
                   0,                                 // ground pressure
                   0,                                 // radio cal
                   0,                                 // accel cal
                   0,                                 // airspeed cal
                   0);                                // unused
}

void Vehicle::startUAVCANBusConfig(void)
{
    sendMavCommand(defaultComponentId(),        // target component
                   MAV_CMD_PREFLIGHT_UAVCAN,    // command id
                   true,                        // showError
                   1);                          // start config
}

void Vehicle::stopUAVCANBusConfig(void)
{
    sendMavCommand(defaultComponentId(),        // target component
                   MAV_CMD_PREFLIGHT_UAVCAN,    // command id
                   true,                        // showError
                   0);                          // stop config
}

void Vehicle::setSoloFirmware(bool soloFirmware)
{
    if (soloFirmware != _soloFirmware) {
        _soloFirmware = soloFirmware;
        emit soloFirmwareChanged(soloFirmware);
    }
}

void Vehicle::motorTest(int motor, int percent, int timeoutSecs, bool showError)
{
    sendMavCommand(_defaultComponentId, MAV_CMD_DO_MOTOR_TEST, showError, motor, MOTOR_TEST_THROTTLE_PERCENT, percent, timeoutSecs, 0, MOTOR_TEST_ORDER_BOARD);
}

QString Vehicle::brandImageIndoor() const
{
    return _firmwarePlugin->brandImageIndoor(this);
}

QString Vehicle::brandImageOutdoor() const
{
    return _firmwarePlugin->brandImageOutdoor(this);
}

void Vehicle::setOfflineEditingDefaultComponentId(int defaultComponentId)
{
    if (_offlineEditingVehicle) {
        _defaultComponentId = defaultComponentId;
    } else {
        qWarning() << "Call to Vehicle::setOfflineEditingDefaultComponentId on vehicle which is not offline";
    }
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
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();
        mavlink_message_t       msg;
        mavlink_logging_ack_t   ack;

        memset(&ack, 0, sizeof(ack));
        ack.sequence = sequence;
        ack.target_component = _defaultComponentId;
        ack.target_system = id();
        mavlink_msg_logging_ack_encode_chan(
                    _mavlink->getSystemId(),
                    _mavlink->getComponentId(),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &ack);
        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
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

QString Vehicle::missionFlightMode() const
{
    return _firmwarePlugin->missionFlightMode();
}

QString Vehicle::pauseFlightMode() const
{
    return _firmwarePlugin->pauseFlightMode();
}

QString Vehicle::rtlFlightMode() const
{
    return _firmwarePlugin->rtlFlightMode();
}

QString Vehicle::smartRTLFlightMode() const
{
    return _firmwarePlugin->smartRTLFlightMode();
}

bool Vehicle::supportsSmartRTL() const
{
    return _firmwarePlugin->supportsSmartRTL();
}

QString Vehicle::landFlightMode() const
{
    return _firmwarePlugin->landFlightMode();
}

QString Vehicle::takeControlFlightMode() const
{
    return _firmwarePlugin->takeControlFlightMode();
}

QString Vehicle::followFlightMode() const
{
    return _firmwarePlugin->followFlightMode();
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

const QVariantList& Vehicle::toolIndicators()
{
    if(_firmwarePlugin) {
        return _firmwarePlugin->toolIndicators(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

const QVariantList& Vehicle::modeIndicators()
{
    if(_firmwarePlugin) {
        return _firmwarePlugin->modeIndicators(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

const QVariantList& Vehicle::staticCameraList() const
{
    if (_firmwarePlugin) {
        return _firmwarePlugin->cameraList(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

void Vehicle::_setupAutoDisarmSignalling()
{
    QString param = _firmwarePlugin->autoDisarmParameter(this);

    if (!param.isEmpty() && _parameterManager->parameterExists(FactSystem::defaultComponentId, param)) {
        Fact* fact = _parameterManager->getParameter(FactSystem::defaultComponentId,param);
        connect(fact, &Fact::rawValueChanged, this, &Vehicle::autoDisarmChanged);
        emit autoDisarmChanged();
    }
}

bool Vehicle::autoDisarm()
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
    mavlink_adsb_vehicle_t adsbVehicleMsg;
    static const int maxTimeSinceLastSeen = 15;

    mavlink_msg_adsb_vehicle_decode(&message, &adsbVehicleMsg);
    if ((adsbVehicleMsg.flags & ADSB_FLAGS_VALID_COORDS) && adsbVehicleMsg.tslc <= maxTimeSinceLastSeen) {
        ADSBVehicle::VehicleInfo_t vehicleInfo;

        vehicleInfo.availableFlags = 0;
        vehicleInfo.icaoAddress = adsbVehicleMsg.ICAO_address;

        vehicleInfo.location.setLatitude(adsbVehicleMsg.lat / 1e7);
        vehicleInfo.location.setLongitude(adsbVehicleMsg.lon / 1e7);
        vehicleInfo.availableFlags |= ADSBVehicle::LocationAvailable;

        vehicleInfo.callsign = adsbVehicleMsg.callsign;
        vehicleInfo.availableFlags |= ADSBVehicle::CallsignAvailable;

        if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_ALTITUDE) {
            vehicleInfo.altitude = (double)adsbVehicleMsg.altitude / 1e3;
            vehicleInfo.availableFlags |= ADSBVehicle::AltitudeAvailable;
        }

        if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_HEADING) {
            vehicleInfo.heading = (double)adsbVehicleMsg.heading / 100.0;
            vehicleInfo.availableFlags |= ADSBVehicle::HeadingAvailable;
        }

        _toolbox->adsbVehicleManager()->adsbVehicleUpdate(vehicleInfo);
    }
}

void Vehicle::_updateDistanceHeadingToHome()
{
    if (coordinate().isValid() && homePosition().isValid()) {
        _distanceToHomeFact.setRawValue(coordinate().distanceTo(homePosition()));
        if (_distanceToHomeFact.rawValue().toDouble() > 1.0) {
            _headingToHomeFact.setRawValue(coordinate().azimuthTo(homePosition()));
        } else {
            _headingToHomeFact.setRawValue(qQNaN());
        }
    } else {
        _distanceToHomeFact.setRawValue(qQNaN());
        _headingToHomeFact.setRawValue(qQNaN());
    }
}

void Vehicle::_updateHeadingToNextWP()
{
    const int currentIndex = _missionManager->currentIndex();
    QList<MissionItem*> llist = _missionManager->missionItems();

    if(llist.size()>currentIndex && currentIndex!=-1
            && llist[currentIndex]->coordinate().longitude()!=0.0
            && coordinate().distanceTo(llist[currentIndex]->coordinate())>5.0 ){

        _headingToNextWPFact.setRawValue(coordinate().azimuthTo(llist[currentIndex]->coordinate()));
    }
    else{
        _headingToNextWPFact.setRawValue(qQNaN());
    }
}

void Vehicle::_updateMissionItemIndex()
{
    const int currentIndex = _missionManager->currentIndex();

    unsigned offset = 0;
    if (!_firmwarePlugin->sendHomePositionToVehicle()) {
        offset = 1;
    }

    _missionItemIndexFact.setRawValue(currentIndex + offset);
}

void Vehicle::_updateDistanceToGCS()
{
    QGeoCoordinate gcsPosition = _toolbox->qgcPositionManager()->gcsPosition();
    if (coordinate().isValid() && gcsPosition.isValid()) {
        _distanceToGCSFact.setRawValue(coordinate().distanceTo(gcsPosition));
    } else {
        _distanceToGCSFact.setRawValue(qQNaN());
    }
}

void Vehicle::_updateHobbsMeter()
{
    _hobbsFact.setRawValue(hobbsMeter());
}

void Vehicle::forceInitialPlanRequestComplete()
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
        QString timeStr = QString::asprintf("%04d:%02d:%02d", hours, minutes, seconds);
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

void Vehicle::_trafficUpdate(bool /*alert*/, QString /*traffic_id*/, QString /*vehicle_id*/, QGeoCoordinate /*location*/, float /*heading*/)
{
#if 0
    // This is ifdef'ed out for now since this code doesn't mesh with the recent ADSB manager changes. Also airmap isn't in any
    // released build. So not going to waste time trying to fix up unused code.
    if (_trafficVehicleMap.contains(traffic_id)) {
        _trafficVehicleMap[traffic_id]->update(alert, location, heading);
    } else {
        ADSBVehicle* vehicle = new ADSBVehicle(location, heading, alert, this);
        _trafficVehicleMap[traffic_id] = vehicle;
        _adsbVehicles.append(vehicle);
    }
#endif
}

void Vehicle::_mavlinkMessageStatus(int uasId, uint64_t totalSent, uint64_t totalReceived, uint64_t totalLoss, float lossPercent)
{
    if(uasId == _id) {
        _mavlinkSentCount       = totalSent;
        _mavlinkReceivedCount   = totalReceived;
        _mavlinkLossCount       = totalLoss;
        _mavlinkLossPercent     = lossPercent;
        emit mavlinkStatusChanged();
    }
}

int  Vehicle::versionCompare(QString& compare)
{
    return _firmwarePlugin->versionCompare(this, compare);
}

int  Vehicle::versionCompare(int major, int minor, int patch)
{
    return _firmwarePlugin->versionCompare(this, major, minor, patch);
}

void Vehicle::_handleMessageInterval(const mavlink_message_t& message)
{
    if (_pidTuningWaitingForRates) {
        mavlink_message_interval_t messageInterval;

        mavlink_msg_message_interval_decode(&message, &messageInterval);

        int msgId = messageInterval.message_id;
        if (_pidTuningMessages.contains(msgId)) {
            _pidTuningMessageRatesUsecs[msgId] = messageInterval.interval_us;
        }

        if (_pidTuningMessageRatesUsecs.count() == _pidTuningMessages.count()) {
            // We have back all the rates we requested
            _pidTuningWaitingForRates = false;
            _pidTuningNextAdjustIndex = 0;
            _pidTuningAdjustRates();
        }
    }
}

void Vehicle::setPIDTuningTelemetryMode(bool pidTuning)
{
    if (pidTuning) {
        if (!_pidTuningTelemetryMode) {
            // First step is to get the current message rates before we adjust them
            _pidTuningTelemetryMode = true;
            _pidTuningWaitingForRates = true;
            _pidTuningMessageRatesUsecs.clear();
            sendMavCommand(defaultComponentId(),
                    MAV_CMD_GET_MESSAGE_INTERVAL,
                    true,                        // show error
                    _pidTuningMessages[0]);
        }
    } else {
        if (_pidTuningTelemetryMode) {
            _pidTuningTelemetryMode = false;
            if (_pidTuningWaitingForRates) {
                // We never finished waiting for previous rates
                _pidTuningWaitingForRates = false;
            } else {
                _pidTuningNextAdjustIndex = 0;
                _pidTuningAdjustRates();
            }
        }
    }
}

void Vehicle::_pidTuningAdjustRates()
{
    int requestedRate = (int)(1000000.0 / 100.0); // 100 Hz in usecs (better set this a bit higher than actually needed,
    // to give it more priority in case of exceeing link bandwidth)

    if (_pidTuningNextAdjustIndex >= _pidTuningMessages.size()) {
        _pidTuningNextAdjustIndex = -1;
        return;
    }
    int telemetry = _pidTuningMessages[_pidTuningNextAdjustIndex];

    if (requestedRate < _pidTuningMessageRatesUsecs[telemetry] || _pidTuningMessageRatesUsecs[telemetry] == 0) {
        sendMavCommand(defaultComponentId(),
                MAV_CMD_SET_MESSAGE_INTERVAL,
                true,                        // show error
                telemetry,
                _pidTuningTelemetryMode ? requestedRate : _pidTuningMessageRatesUsecs[telemetry]);
    }

    if (_pidTuningNextAdjustIndex == 0) {
        setLiveUpdates(_pidTuningTelemetryMode);
        _setpointFactGroup.setLiveUpdates(_pidTuningTelemetryMode);
    }
    ++_pidTuningNextAdjustIndex;
}

void Vehicle::_initializeCsv()
{
    if(!_toolbox->settingsManager()->appSettings()->saveCsvTelemetry()->rawValue().toBool()){
        return;
    }
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    QString fileName = QString("%1 vehicle%2.csv").arg(now).arg(_id);
    QDir saveDir(_toolbox->settingsManager()->appSettings()->telemetrySavePath());
    _csvLogFile.setFileName(saveDir.absoluteFilePath(fileName));

    if (!_csvLogFile.open(QIODevice::Append)) {
        qCWarning(VehicleLog) << "unable to open file for csv logging, Stopping csv logging!";
        return;
    }

    QTextStream stream(&_csvLogFile);
    QStringList allFactNames;
    allFactNames << factNames();
    for (const QString& groupName: factGroupNames()) {
        for(const QString& factName: getFactGroup(groupName)->factNames()){
            allFactNames << QString("%1.%2").arg(groupName, factName);
        }
    }
    qCDebug(VehicleLog) << "Facts logged to csv:" << allFactNames;
    stream << "Timestamp," << allFactNames.join(",") << "\n";
}

void Vehicle::_writeCsvLine()
{
    // Only save the logs after the the vehicle gets armed, unless "Save logs even if vehicle was not armed" is checked
    if(!_csvLogFile.isOpen() &&
            (_armed || _toolbox->settingsManager()->appSettings()->telemetrySaveNotArmed()->rawValue().toBool())){
        _initializeCsv();
    }

    if(!_csvLogFile.isOpen()){
        return;
    }

    QStringList allFactValues;
    QTextStream stream(&_csvLogFile);

    // Write timestamp to csv file
    allFactValues << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    // Write Vehicle's own facts
    for (const QString& factName : factNames()) {
        allFactValues << getFact(factName)->cookedValueString();
    }
    // write facts from Vehicle's FactGroups
    for (const QString& groupName: factGroupNames()) {
        for (const QString& factName : getFactGroup(groupName)->factNames()) {
            allFactValues << getFactGroup(groupName)->getFact(factName)->cookedValueString();
        }
    }

    stream << allFactValues.join(",") << "\n";
}

#if !defined(NO_ARDUPILOT_DIALECT)
void Vehicle::flashBootloader()
{
    sendMavCommand(defaultComponentId(),
                   MAV_CMD_FLASH_BOOTLOADER,
                   true,        // show error
                   0, 0, 0, 0,  // param 1-4 not used
                   290876);     // magic number

}
#endif

void Vehicle::gimbalControlValue(double pitch, double yaw)
{
    //qDebug() << "Gimbal:" << pitch << yaw;
    sendMavCommand(
                _defaultComponentId,
                MAV_CMD_DO_MOUNT_CONTROL,
                false,                               // show errors
                static_cast<float>(pitch),           // Pitch 0 - 90
                0,                                   // Roll (not used)
                static_cast<float>(yaw),             // Yaw -180 - 180
                0,                                   // Altitude (not used)
                0,                                   // Latitude (not used)
                0,                                   // Longitude (not used)
                MAV_MOUNT_MODE_MAVLINK_TARGETING);   // MAVLink Roll,Pitch,Yaw
}

void Vehicle::gimbalPitchStep(int direction)
{
    if(_haveGimbalData) {
        //qDebug() << "Pitch:" << _curGimbalPitch << direction << (_curGimbalPitch + direction);
        double p = static_cast<double>(_curGimbalPitch + direction);
        gimbalControlValue(p, static_cast<double>(_curGinmbalYaw));
    }
}

void Vehicle::gimbalYawStep(int direction)
{
    if(_haveGimbalData) {
        //qDebug() << "Yaw:" << _curGinmbalYaw << direction << (_curGinmbalYaw + direction);
        double y = static_cast<double>(_curGinmbalYaw + direction);
        gimbalControlValue(static_cast<double>(_curGimbalPitch), y);
    }
}

void Vehicle::centerGimbal()
{
    if(_haveGimbalData) {
        gimbalControlValue(0.0, 0.0);
    }
}

void Vehicle::_handleGimbalOrientation(const mavlink_message_t& message)
{
    mavlink_mount_orientation_t o;
    mavlink_msg_mount_orientation_decode(&message, &o);
    if(fabsf(_curGimbalRoll - o.roll) > 0.5f) {
        _curGimbalRoll = o.roll;
        emit gimbalRollChanged();
    }
    if(fabsf(_curGimbalPitch - o.pitch) > 0.5f) {
        _curGimbalPitch = o.pitch;
        emit gimbalPitchChanged();
    }
    if(fabsf(_curGinmbalYaw - o.yaw) > 0.5f) {
        _curGinmbalYaw = o.yaw;
        emit gimbalYawChanged();
    }
    if(!_haveGimbalData) {
        _haveGimbalData = true;
        emit gimbalDataChanged();
    }
}

void Vehicle::_handleObstacleDistance(const mavlink_message_t& message)
{
    mavlink_obstacle_distance_t o;
    mavlink_msg_obstacle_distance_decode(&message, &o);
    _objectAvoidance->update(&o);
}

void Vehicle::updateFlightDistance(double distance)
{
    _flightDistanceFact.setRawValue(_flightDistanceFact.rawValue().toDouble() + distance);
}

void Vehicle::sendParamMapRC(const QString& paramName, double scale, double centerValue, int tuningID, double minValue, double maxValue)
{
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();
        mavlink_message_t       message;

        char param_id_cstr[MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN] = {};
        // Copy string into buffer, ensuring not to exceed the buffer size
        for (unsigned int i = 0; i < sizeof(param_id_cstr); i++) {
            if ((int)i < paramName.length()) {
                param_id_cstr[i] = paramName.toLatin1()[i];
            }
        }

        mavlink_msg_param_map_rc_pack_chan(static_cast<uint8_t>(_mavlink->getSystemId()),
                                           static_cast<uint8_t>(_mavlink->getComponentId()),
                                           sharedLink->mavlinkChannel(),
                                           &message,
                                           _id,
                                           MAV_COMP_ID_AUTOPILOT1,
                                           param_id_cstr,
                                           -1,                                                  // parameter name specified as string in previous argument
                                           static_cast<uint8_t>(tuningID),
                                           static_cast<float>(scale),
                                           static_cast<float>(centerValue),
                                           static_cast<float>(minValue),
                                           static_cast<float>(maxValue));
        sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }
}

void Vehicle::clearAllParamMapRC(void)
{
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();
        char param_id_cstr[MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN] = {};

        for (int i = 0; i < 3; i++) {
            mavlink_message_t message;
            mavlink_msg_param_map_rc_pack_chan(static_cast<uint8_t>(_mavlink->getSystemId()),
                                               static_cast<uint8_t>(_mavlink->getComponentId()),
                                               sharedLink->mavlinkChannel(),
                                               &message,
                                               _id,
                                               MAV_COMP_ID_AUTOPILOT1,
                                               param_id_cstr,
                                               -2,                                                  // Disable map for specified tuning id
                                               i,                                                   // tuning id
                                               0, 0, 0, 0);                                         // unused
            sendMessageOnLinkThreadSafe(sharedLink.get(), message);
        }
    }
}

void Vehicle::sendJoystickDataThreadSafe(float roll, float pitch, float yaw, float thrust, quint16 buttons)
{
    WeakLinkInterfacePtr weakLink = vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        if (sharedLink->linkConfiguration()->isHighLatency()) {
            return;
        }

        mavlink_message_t message;

        // Incoming values are in the range -1:1
        float axesScaling =         1.0 * 1000.0;
        float newRollCommand =      roll * axesScaling;
        float newPitchCommand  =    pitch * axesScaling;    // Joystick data is reverse of mavlink values
        float newYawCommand    =    yaw * axesScaling;
        float newThrustCommand =    thrust * axesScaling;

        mavlink_msg_manual_control_pack_chan(
                    static_cast<uint8_t>(_mavlink->getSystemId()),
                    static_cast<uint8_t>(_mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &message,
                    static_cast<uint8_t>(_id),
                    static_cast<int16_t>(newPitchCommand),
                    static_cast<int16_t>(newRollCommand),
                    static_cast<int16_t>(newThrustCommand),
                    static_cast<int16_t>(newYawCommand),
                    buttons);
        sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }
}

void Vehicle::triggerSimpleCamera()
{
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_DO_DIGICAM_CONTROL,
                   true,                        // show errors
                   0.0, 0.0, 0.0, 0.0,          // param 1-4 unused
                   1.0);                        // trigger camera
}
