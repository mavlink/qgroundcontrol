#include "Vehicle.h"
#include "Actuators.h"
#include "BatteryFactGroupListModel.h"
#include "EscStatusFactGroupListModel.h"
#include "RadioStatusFactGroup.h"
#include "TerrainFactGroup.h"
#include "VehicleClockFactGroup.h"
#include "VehicleDistanceSensorFactGroup.h"
#include "VehicleEFIFactGroup.h"
#include "VehicleEstimatorStatusFactGroup.h"
#include "VehicleGeneratorFactGroup.h"
#include "VehicleGPS2FactGroup.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleGPSAggregateFactGroup.h"
#include "VehicleHygrometerFactGroup.h"
#include "VehicleLocalPositionFactGroup.h"
#include "VehicleLocalPositionSetpointFactGroup.h"
#include "VehicleRPMFactGroup.h"
#include "VehicleSetpointFactGroup.h"
#include "VehicleTemperatureFactGroup.h"
#include "VehicleVibrationFactGroup.h"
#include "VehicleWindFactGroup.h"
#include "VehicleSupports.h"
#include "ADSBVehicleManager.h"
#include "AudioOutput.h"
#include "AutoPilotPlugin.h"
#include "ComponentInformationManager.h"
#include "MAVLinkEventManager.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "FTPManager.h"
#include "GeoFenceManager.h"
#include "ImageProtocolManager.h"
#include "InitialConnectStateMachine.h"
#include "Joystick.h"
#include "JoystickManager.h"
#include "LinkManager.h"
#include "MavCommandQueue.h"
#include "MessageIntervalManager.h"
#include "TerrainQueryCoordinator.h"
#include "MAVLinkLogManager.h"
#include "MAVLinkProtocol.h"
#include "MissionCommandTree.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "PlanMasterController.h"
#include "PositionManager.h"
#include "AppMessages.h"
#include "QGCMath.h"
#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "QGCCorePlugin.h"
#include "QGCImageProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCQGeoCoordinate.h"
#include "RallyPointManager.h"
#include "RemoteIDManager.h"
#include "RequestMessageCoordinator.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "FlyViewSettings.h"
#include "StandardModes.h"
#include "TerrainProtocolHandler.h"
#include "TerrainQuery.h"
#include "TrajectoryPoints.h"
#include "VehicleLinkManager.h"
#include "MAVLinkStreamConfig.h"
#include "QGCMapCircle.h"
#include "QmlObjectListModel.h"
#include "SysStatusSensorInfo.h"
#include "VehicleObjectAvoidance.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "QGCSensors.h"
#include "StatusTextHandler.h"
#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "GimbalController.h"
#include "MavlinkSettings.h"
#include "APM.h"

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

#include <QtCore/QDateTime>

QGC_LOGGING_CATEGORY(VehicleLog, "Vehicle.Vehicle")

#define UPDATE_TIMER 50
#define DEFAULT_LAT  38.965767f
#define DEFAULT_LON -120.083923f

// After a second GCS has requested control and we have given it permission to takeover, we will remove takeover permission automatically after this timeout
// If the second GCS didn't get control
#define REQUEST_OPERATOR_CONTROL_ALLOW_TAKEOVER_TIMEOUT_MSECS 10000

const QString guided_mode_not_supported_by_vehicle = QObject::tr("Guided mode not supported by Vehicle.");

// Standard connected vehicle
Vehicle::Vehicle(LinkInterface*             link,
                 int                        vehicleId,
                 int                        defaultComponentId,
                 MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 QObject*                   parent)
    : VehicleFactGroup              (parent)
    , _systemID                     (vehicleId)
    , _defaultComponentId           (defaultComponentId)
    , _firmwareType                 (firmwareType)
    , _vehicleType                  (vehicleType)
    , _defaultCruiseSpeed           (SettingsManager::instance()->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed            (SettingsManager::instance()->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _sysStatusSensorInfo          (std::make_unique<SysStatusSensorInfo>(this))
    , _trajectoryPoints             (new TrajectoryPoints(this, this))
    , _cameraTriggerPoints          (std::make_unique<QmlObjectListModel>(this))
    , _orbitMapCircle               (std::make_unique<QGCMapCircle>(this))
    , _mavlinkStreamConfig          (std::make_unique<MAVLinkStreamConfig>(std::bind(&Vehicle::_setMessageInterval, this, std::placeholders::_1, std::placeholders::_2)))
    , _vehicleFactGroup             (this)
{
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &Vehicle::_activeVehicleChanged);

    connect(MAVLinkProtocol::instance(), &MAVLinkProtocol::messageReceived,        this, &Vehicle::_mavlinkMessageReceived);
    connect(MAVLinkProtocol::instance(), &MAVLinkProtocol::mavlinkMessageStatus,   this, &Vehicle::_mavlinkMessageStatus);

    connect(this, &Vehicle::flightModeChanged,          this, &Vehicle::_handleFlightModeChanged);
    connect(this, &Vehicle::armedChanged,               this, &Vehicle::_announceArmedChanged);
    connect(this, &Vehicle::flyingChanged, this, [this](bool flying){
        if (flying) {
            setInitialGCSPressure(QGCSensors::QGCPressure::instance()->pressure());
            setInitialGCSTemperature(QGCSensors::QGCPressure::instance()->temperature());
        }
    });

    connect(MultiVehicleManager::instance(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &Vehicle::_vehicleParamLoaded);

    _commonInit(link);

    // Set video stream to udp if running ArduSub and Video is disabled
    if (sub() && SettingsManager::instance()->videoSettings()->videoSource()->rawValue() == VideoSettings::videoDisabled) {
        SettingsManager::instance()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
        SettingsManager::instance()->videoSettings()->lowLatencyMode()->setRawValue(true);
    }

    _autopilotPlugin = _firmwarePlugin->autopilotPlugin(this);
    _autopilotPlugin->setParent(this);

    // PreArm Error self-destruct timer
    connect(&_prearmErrorTimer, &QTimer::timeout, this, &Vehicle::_prearmErrorTimeout);
    _prearmErrorTimer.setInterval(_prearmErrorTimeoutMSecs);
    _prearmErrorTimer.setSingleShot(true);

    // Command queue timer is managed by MavCommandQueue itself.

    // MAV_TYPE_GENERIC is used by unit test for creating a vehicle which doesn't do the connect sequence. This
    // way we can test the methods that are used within the connect sequence.
    if (!QGC::runningUnitTests() || _vehicleType != MAV_TYPE_GENERIC) {
        _initialConnectStateMachine->start();
    }

    _firmwarePlugin->initializeVehicle(this);
    for(auto& factName: factNames()) {
        _firmwarePlugin->adjustMetaData(vehicleType, getFact(factName)->metaData());
    }

    _sendMultipleTimer.start(_sendMessageMultipleIntraMessageDelay);
    connect(&_sendMultipleTimer, &QTimer::timeout, this, &Vehicle::_sendMessageMultipleNext);

    connect(&_orbitTelemetryTimer, &QTimer::timeout, this, &Vehicle::_orbitTelemetryTimeout);

    // Start csv logger
    connect(&_csvLogTimer, &QTimer::timeout, this, &Vehicle::_writeCsvLine);
    _csvLogTimer.start(1000);

}

// Disconnected Vehicle for offline editing
Vehicle::Vehicle(MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 QObject*                   parent)
    : VehicleFactGroup                  (parent)
    , _systemID                         (0)
    , _defaultComponentId               (MAV_COMP_ID_ALL)
    , _offlineEditingVehicle            (true)
    , _firmwareType                     (firmwareType)
    , _vehicleType                      (vehicleType)
    , _defaultCruiseSpeed               (SettingsManager::instance()->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed                (SettingsManager::instance()->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _capabilityBitsKnown              (true)
    , _capabilityBits                   (MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY)
    , _sysStatusSensorInfo              (std::make_unique<SysStatusSensorInfo>(this))
    , _trajectoryPoints                 (new TrajectoryPoints(this, this))
    , _cameraTriggerPoints              (std::make_unique<QmlObjectListModel>(this))
    , _orbitMapCircle                   (std::make_unique<QGCMapCircle>(this))
    , _mavlinkStreamConfig              (std::make_unique<MAVLinkStreamConfig>(std::bind(&Vehicle::_setMessageInterval, this, std::placeholders::_1, std::placeholders::_2)))
    , _vehicleFactGroup                 (this)
{
    // This will also set the settings based firmware/vehicle types. So it needs to happen first.
    if (_firmwareType == MAV_AUTOPILOT_TRACK) {
        trackFirmwareVehicleTypeChanges();
    }

    _commonInit(nullptr /* link */);

    connect(SettingsManager::instance()->appSettings()->offlineEditingCruiseSpeed(),   &Fact::rawValueChanged, this, &Vehicle::_offlineCruiseSpeedSettingChanged);
    connect(SettingsManager::instance()->appSettings()->offlineEditingHoverSpeed(),    &Fact::rawValueChanged, this, &Vehicle::_offlineHoverSpeedSettingChanged);

    _offlineFirmwareTypeSettingChanged(_firmwareType);  // This adds correct terrain capability bit
    _firmwarePlugin->initializeVehicle(this);
}

void Vehicle::trackFirmwareVehicleTypeChanges(void)
{
    connect(SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass(), &Fact::rawValueChanged, this, &Vehicle::_offlineFirmwareTypeSettingChanged);
    connect(SettingsManager::instance()->appSettings()->offlineEditingVehicleClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineVehicleTypeSettingChanged);

    _offlineFirmwareTypeSettingChanged(SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass()->rawValue());
    _offlineVehicleTypeSettingChanged(SettingsManager::instance()->appSettings()->offlineEditingVehicleClass()->rawValue());
}

void Vehicle::stopTrackingFirmwareVehicleTypeChanges(void)
{
    disconnect(SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineFirmwareTypeSettingChanged);
    disconnect(SettingsManager::instance()->appSettings()->offlineEditingVehicleClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineVehicleTypeSettingChanged);
}

void Vehicle::_commonInit(LinkInterface* link)
{
    _firmwarePlugin = FirmwarePluginManager::instance()->firmwarePluginForAutopilot(_firmwareType, _vehicleType);

    connect(_firmwarePlugin, &FirmwarePlugin::toolIndicatorsChanged, this, &Vehicle::toolIndicatorsChanged);

    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateDistanceHeadingHome);
    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateDistanceHeadingGCS);
    connect(this, &Vehicle::homePositionChanged,    this, &Vehicle::_updateDistanceHeadingHome);
    connect(this, &Vehicle::hobbsMeterChanged,      this, &Vehicle::_updateHobbsMeter);
    connect(this, &Vehicle::coordinateChanged, _terrainQueryCoordinator, &TerrainQueryCoordinator::updateAltAboveTerrain);
    connect(this, &Vehicle::vehicleTypeChanged,     this, &Vehicle::inFwdFlightChanged);
    connect(this, &Vehicle::vtolInFwdFlightChanged, this, &Vehicle::inFwdFlightChanged);

    connect(QGCPositionManager::instance(), &QGCPositionManager::gcsPositionChanged, this, &Vehicle::_updateDistanceHeadingGCS);
    connect(QGCPositionManager::instance(), &QGCPositionManager::gcsPositionChanged, this, &Vehicle::_updateHomepoint);

    _missionManager = new MissionManager(this);
    connect(_missionManager, &MissionManager::error,                    this, &Vehicle::_missionManagerError);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_firstMissionLoadComplete);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_clearCameraTriggerPoints);
    connect(_missionManager, &MissionManager::sendComplete,             this, &Vehicle::_clearCameraTriggerPoints);
    connect(_missionManager, &MissionManager::currentIndexChanged,      this, &Vehicle::_updateHeadingToNextWP);
    connect(_missionManager, &MissionManager::currentIndexChanged,      this, &Vehicle::_updateMissionItemIndex);

    connect(_missionManager, &MissionManager::sendComplete,             _trajectoryPoints, &TrajectoryPoints::clear);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, _trajectoryPoints, &TrajectoryPoints::clear);

    _standardModes                  = new StandardModes                 (this, this);
    _componentInformationManager    = new ComponentInformationManager   (this, this);
    _initialConnectStateMachine     = new InitialConnectStateMachine    (this, this);
    _ftpManager                     = new FTPManager                    (this);

    // Command send/ack queue and request-message coordinator must exist before any
    // manager that may call Vehicle::sendMavCommand / requestMessage during construction
    // (e.g. VehicleLinkManager::_addLink() → _updatePrimaryLink → sendMavCommand).
    _mavCmdQueue = new MavCommandQueue(this);
    connect(_mavCmdQueue, &MavCommandQueue::commandResult, this, &Vehicle::mavCommandResult);
    _reqMsgCoord = new RequestMessageCoordinator(this, _mavCmdQueue);
    _messageIntervalManager = new MessageIntervalManager(this, _mavCmdQueue, _reqMsgCoord);
    connect(_messageIntervalManager, &MessageIntervalManager::mavlinkMsgIntervalsChanged,
            this, &Vehicle::mavlinkMsgIntervalsChanged);
    _terrainQueryCoordinator = new TerrainQueryCoordinator(this);

    _vehicleLinkManager = new VehicleLinkManager(this);
    if (link) {
        _vehicleLinkManager->_addLink(link);
    }

    connect(_standardModes, &StandardModes::modesUpdated, this, &Vehicle::flightModesChanged);
    // Re-emit flightModeChanged after available modes mapping updates so UI refreshes
    // the human-readable mode name even if HEARTBEAT arrived earlier.
    connect(_standardModes, &StandardModes::modesUpdated, this, [this]() {
        emit flightModeChanged(flightMode());
    });

    _parameterManager = new ParameterManager(this);
    connect(_parameterManager, &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);
    connect(_parameterManager, &ParameterManager::parametersReadyChanged, this, [this](bool) {
        emit hasGripperChanged();
    });
    connect(_initialConnectStateMachine, &InitialConnectStateMachine::progressUpdate,
            this, &Vehicle::_gotProgressUpdate);
    connect(_parameterManager, &ParameterManager::loadProgressChanged, this, &Vehicle::_gotProgressUpdate);

    _objectAvoidance = new VehicleObjectAvoidance(this, this);

    _autotune = _firmwarePlugin->createAutotune(this);

    // GeoFenceManager needs to access ParameterManager so make sure to create after
    _geoFenceManager = new GeoFenceManager(this);
    connect(_geoFenceManager, &GeoFenceManager::error,          this, &Vehicle::_geoFenceManagerError);
    connect(_geoFenceManager, &GeoFenceManager::loadComplete,   this, &Vehicle::_firstGeoFenceLoadComplete);

    _rallyPointManager = new RallyPointManager(this);
    connect(_rallyPointManager, &RallyPointManager::error,          this, &Vehicle::_rallyPointManagerError);
    connect(_rallyPointManager, &RallyPointManager::loadComplete,   this, &Vehicle::_firstRallyPointLoadComplete);

    // Remote ID manager might want to acces parameters so make sure to create it after
    _remoteIDManager = new RemoteIDManager(this);

    // Flight modes can differ based on advanced mode
    connect(QGCCorePlugin::instance(), &QGCCorePlugin::showAdvancedUIChanged, this, &Vehicle::flightModesChanged);

    _gpsFactGroup                   = new VehicleGPSFactGroup(this);
    _gps2FactGroup                  = new VehicleGPS2FactGroup(this);
    _gpsAggregateFactGroup          = new VehicleGPSAggregateFactGroup(this);
    _windFactGroup                  = new VehicleWindFactGroup(this);
    _vibrationFactGroup             = new VehicleVibrationFactGroup(this);
    _temperatureFactGroup           = new VehicleTemperatureFactGroup(this);
    _clockFactGroup                 = new VehicleClockFactGroup(this);
    _setpointFactGroup              = new VehicleSetpointFactGroup(this);
    _distanceSensorFactGroup        = new VehicleDistanceSensorFactGroup(this);
    _localPositionFactGroup         = new VehicleLocalPositionFactGroup(this);
    _localPositionSetpointFactGroup = new VehicleLocalPositionSetpointFactGroup(this);
    _estimatorStatusFactGroup       = new VehicleEstimatorStatusFactGroup(this);
    _hygrometerFactGroup            = new VehicleHygrometerFactGroup(this);
    _generatorFactGroup             = new VehicleGeneratorFactGroup(this);
    _efiFactGroup                   = new VehicleEFIFactGroup(this);
    _rpmFactGroup                   = new VehicleRPMFactGroup(this);
    _terrainFactGroup               = new TerrainFactGroup(this);
    _radioStatusFactGroup           = new RadioStatusFactGroup(this);
    _batteryFactGroupListModel      = new BatteryFactGroupListModel(this);
    _escStatusFactGroupListModel    = new EscStatusFactGroupListModel(this);

    if (!_offlineEditingVehicle) {
        _terrainProtocolHandler = new TerrainProtocolHandler(this, _terrainFactGroup, this);
    }

    _gpsAggregateFactGroup->bindToGps(_gpsFactGroup, _gps2FactGroup);

    _createImageProtocolManager();
    _createStatusTextHandler();
    _createMAVLinkLogManager();
    _createMAVLinkEventManager();

    // _addFactGroup(_vehicleFactGroup,            _vehicleFactGroupName);
    _addFactGroup(_gpsFactGroup,               _gpsFactGroupName);
    _addFactGroup(_gps2FactGroup,              _gps2FactGroupName);
    _addFactGroup(_gpsAggregateFactGroup,      _gpsAggregateFactGroupName);
    _addFactGroup(_windFactGroup,              _windFactGroupName);
    _addFactGroup(_vibrationFactGroup,         _vibrationFactGroupName);
    _addFactGroup(_temperatureFactGroup,       _temperatureFactGroupName);
    _addFactGroup(_clockFactGroup,             _clockFactGroupName);
    _addFactGroup(_setpointFactGroup,          _setpointFactGroupName);
    _addFactGroup(_distanceSensorFactGroup,    _distanceSensorFactGroupName);
    _addFactGroup(_localPositionFactGroup,     _localPositionFactGroupName);
    _addFactGroup(_localPositionSetpointFactGroup,_localPositionSetpointFactGroupName);
    _addFactGroup(_estimatorStatusFactGroup,   _estimatorStatusFactGroupName);
    _addFactGroup(_hygrometerFactGroup,        _hygrometerFactGroupName);
    _addFactGroup(_generatorFactGroup,         _generatorFactGroupName);
    _addFactGroup(_efiFactGroup,               _efiFactGroupName);
    _addFactGroup(_rpmFactGroup,               _rpmFactGroupName);
    _addFactGroup(_terrainFactGroup,           _terrainFactGroupName);
    _addFactGroup(_radioStatusFactGroup,       _radioStatusFactGroupName);

    // Add firmware-specific fact groups, if provided
    QMap<QString, FactGroup*>* fwFactGroups = _firmwarePlugin->factGroups();
    if (fwFactGroups) {
        for (auto it = fwFactGroups->keyValueBegin(); it != fwFactGroups->keyValueEnd(); ++it) {
            _addFactGroup(it->second, it->first);
        }
    }

    _flightTimeUpdater.setInterval(1000);
    _flightTimeUpdater.setSingleShot(false);
    connect(&_flightTimeUpdater, &QTimer::timeout, this, &Vehicle::_updateFlightTime);

    // Set video stream to udp if running ArduSub and Video is disabled
    if (sub() && SettingsManager::instance()->videoSettings()->videoSource()->rawValue() == VideoSettings::videoDisabled) {
        SettingsManager::instance()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
        SettingsManager::instance()->videoSettings()->lowLatencyMode()->setRawValue(true);
    }

    _gimbalController = new GimbalController(this);
    _vehicleSupports = new VehicleSupports(this);

    _createCameraManager();
}

Vehicle::~Vehicle()
{
    qCDebug(VehicleLog) << "~Vehicle" << this;

    // Stop all timers and disconnect their signals to prevent any callbacks during destruction.
    // Even though _stopCommandProcessing() should have been called earlier via VehicleLinkManager,
    // we do it again here defensively in case the vehicle is destroyed without going through
    // the normal link removal path (e.g., in unit tests).
    if (_mavCmdQueue) {
        _mavCmdQueue->stop();
    }
    _sendMultipleTimer.stop();
    _sendMultipleTimer.disconnect();
    _prearmErrorTimer.stop();
    _prearmErrorTimer.disconnect();

    delete _missionManager;
    _missionManager = nullptr;

    delete _autopilotPlugin;
    _autopilotPlugin = nullptr;
}

FactGroup* Vehicle::gpsFactGroup()                  { return _gpsFactGroup; }
FactGroup* Vehicle::gps2FactGroup()                 { return _gps2FactGroup; }
FactGroup* Vehicle::gpsAggregateFactGroup()         { return _gpsAggregateFactGroup; }
FactGroup* Vehicle::windFactGroup()                 { return _windFactGroup; }
FactGroup* Vehicle::vibrationFactGroup()            { return _vibrationFactGroup; }
FactGroup* Vehicle::temperatureFactGroup()          { return _temperatureFactGroup; }
FactGroup* Vehicle::clockFactGroup()                { return _clockFactGroup; }
FactGroup* Vehicle::setpointFactGroup()             { return _setpointFactGroup; }
FactGroup* Vehicle::distanceSensorFactGroup()       { return _distanceSensorFactGroup; }
FactGroup* Vehicle::localPositionFactGroup()        { return _localPositionFactGroup; }
FactGroup* Vehicle::localPositionSetpointFactGroup() { return _localPositionSetpointFactGroup; }
FactGroup* Vehicle::estimatorStatusFactGroup()      { return _estimatorStatusFactGroup; }
FactGroup* Vehicle::terrainFactGroup()              { return _terrainFactGroup; }
FactGroup* Vehicle::hygrometerFactGroup()           { return _hygrometerFactGroup; }
FactGroup* Vehicle::generatorFactGroup()            { return _generatorFactGroup; }
FactGroup* Vehicle::efiFactGroup()                  { return _efiFactGroup; }
FactGroup* Vehicle::rpmFactGroup()                  { return _rpmFactGroup; }
FactGroup* Vehicle::radioStatusFactGroup()          { return _radioStatusFactGroup; }

QmlObjectListModel* Vehicle::batteries()            { return _batteryFactGroupListModel; }
QmlObjectListModel* Vehicle::escs()                 { return _escStatusFactGroupListModel; }

QObject* Vehicle::sysStatusSensorInfo()                             { return _sysStatusSensorInfo.get(); }
QmlObjectListModel* Vehicle::cameraTriggerPoints()                  { return _cameraTriggerPoints.get(); }
void Vehicle::closeVehicle()                                        { _vehicleLinkManager->closeVehicle(); }

void Vehicle::_deleteCameraManager()
{
    if(_cameraManager) {
        // Disconnect all signals to prevent any callbacks during or after deletion
        _cameraManager->disconnect();
        delete _cameraManager;
        _cameraManager = nullptr;
    }
}

void Vehicle::_deleteGimbalController()
{
    if (_gimbalController) {
        // Disconnect all signals to prevent any callbacks during or after deletion
        _gimbalController->disconnect();
        delete _gimbalController;
        _gimbalController = nullptr;
    }
}

void Vehicle::_stopCommandProcessing()
{
    qCDebug(VehicleLog) << "_stopCommandProcessing - stopping timers and clearing pending commands";

    // Stop timers AND disconnect their signals to prevent any pending callbacks
    // from being delivered after this point. This is critical during vehicle destruction
    // where a queued callback could access a partially-destroyed vehicle.
    if (_mavCmdQueue) {
        _mavCmdQueue->stop();
    }
    if (_reqMsgCoord) {
        _reqMsgCoord->stop();
    }
    _sendMultipleTimer.stop();
    _sendMultipleTimer.disconnect();
}

void Vehicle::_offlineFirmwareTypeSettingChanged(QVariant varFirmwareType)
{
    _firmwareType = static_cast<MAV_AUTOPILOT>(varFirmwareType.toInt());
    _firmwarePlugin = FirmwarePluginManager::instance()->firmwarePluginForAutopilot(_firmwareType, _vehicleType);
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
    return QGCMAVLink::firmwareClassToString(_firmwareType);
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
    if (message.sysid != _systemID && message.sysid != 0) {
        // We allow RADIO_STATUS messages which come from a link the vehicle is using to pass through and be handled
        if (!(message.msgid == MAVLINK_MSG_ID_RADIO_STATUS && _vehicleLinkManager->containsLink(link))) {
            return;
        }
    }

    // Try to auto-detect signing key from incoming signed packets
    if (MAVLinkSigning::isMessageSigned(message) && !MAVLinkSigning::isSigningEnabled(static_cast<mavlink_channel_t>(link->mavlinkChannel()))) {
        const QString detectedKeyName = MAVLinkSigning::tryDetectKey(static_cast<mavlink_channel_t>(link->mavlinkChannel()), message);
        if (!detectedKeyName.isEmpty() && link == vehicleLinkManager()->primaryLink().lock().get()) {
            _mavlinkSigningKeyName = detectedKeyName;
            emit mavlinkSigningChanged();
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
    if (!QGCCorePlugin::instance()->mavlinkMessage(this, link, message)) {
        return;
    }

    if (!_terrainProtocolHandler->mavlinkMessageReceived(message)) {
        return;
    }
    _ftpManager->_mavlinkMessageReceived(message);
    _parameterManager->mavlinkMessageReceived(message);
    (void) QMetaObject::invokeMethod(_imageProtocolManager, "mavlinkMessageReceived", Qt::AutoConnection, message);
    _remoteIDManager->mavlinkMessageReceived(message);

    _reqMsgCoord->handleReceivedMessage(message);

    // Handle creation of dynamic fact group lists
    _batteryFactGroupListModel->handleMessageForFactGroupCreation(this, message);
    _escStatusFactGroupListModel->handleMessageForFactGroupCreation(this, message);

    // Let the fact groups take a whack at the mavlink traffic
    for (FactGroup* factGroup : factGroups()) {
        factGroup->handleMessage(this, message);
    }

    this->handleMessage(this, message);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HOME_POSITION:
        _handleHomePosition(message);
        break;
    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartbeat(message);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS:
        _handleRCChannels(message);
        break;
    case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
    {
        mavlink_servo_output_raw_t servoOutputRaw;
        mavlink_msg_servo_output_raw_decode(&message, &servoOutputRaw);

        // ArduPilot commonly publishes servo1_raw..servo16_raw in a single packet (port may remain 0).
        const uint16_t rawValues[16] = {
            servoOutputRaw.servo1_raw,
            servoOutputRaw.servo2_raw,
            servoOutputRaw.servo3_raw,
            servoOutputRaw.servo4_raw,
            servoOutputRaw.servo5_raw,
            servoOutputRaw.servo6_raw,
            servoOutputRaw.servo7_raw,
            servoOutputRaw.servo8_raw,
            servoOutputRaw.servo9_raw,
            servoOutputRaw.servo10_raw,
            servoOutputRaw.servo11_raw,
            servoOutputRaw.servo12_raw,
            servoOutputRaw.servo13_raw,
            servoOutputRaw.servo14_raw,
            servoOutputRaw.servo15_raw,
            servoOutputRaw.servo16_raw
        };

        for (int servoIndex = 0; servoIndex < _servoOutputRawValues.size() && servoIndex < 16; servoIndex++) {
            _servoOutputRawValues[servoIndex] = (rawValues[servoIndex] == UINT16_MAX) ? -1 : static_cast<int>(rawValues[servoIndex]);
        }

        emit servoOutputsChanged(_servoOutputRawValues);
    }
        break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
        _handleBatteryStatus(message);
        break;
    case MAVLINK_MSG_ID_SYS_STATUS:
        _handleSysStatus(message);
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
    case MAVLINK_MSG_ID_CAMERA_IMAGE_CAPTURED:
        _handleCameraImageCaptured(message);
        break;
    case MAVLINK_MSG_ID_ADSB_VEHICLE:
        ADSBVehicleManager::instance()->mavlinkMessageReceived(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(message);
        break;
    case MAVLINK_MSG_ID_STATUSTEXT:
        m_statusTextHandler->mavlinkMessageReceived(message);
        break;
    case MAVLINK_MSG_ID_ORBIT_EXECUTION_STATUS:
        _handleOrbitExecutionStatus(message);
        break;
    case MAVLINK_MSG_ID_PING:
        _handlePing(link, message);
        break;
    case MAVLINK_MSG_ID_OBSTACLE_DISTANCE:
        _handleObstacleDistance(message);
        break;
    case MAVLINK_MSG_ID_FENCE_STATUS:
        _handleFenceStatus(message);
        break;

    case MAVLINK_MSG_ID_EVENT:
    case MAVLINK_MSG_ID_CURRENT_EVENT_SEQUENCE:
    case MAVLINK_MSG_ID_RESPONSE_EVENT_ERROR:
        _handleEventMessage(message);
        break;

    case MAVLINK_MSG_ID_SERIAL_CONTROL:
    {
        mavlink_serial_control_t ser;
        mavlink_msg_serial_control_decode(&message, &ser);
        if (static_cast<size_t>(ser.count) > sizeof(ser.data)) {
            qCWarning(VehicleLog) << "Invalid count for SERIAL_CONTROL, discarding." << ser.count;
        } else {
            emit mavlinkSerialControl(ser.device, ser.flags, ser.timeout, ser.baudrate,
                    QByteArray(reinterpret_cast<const char*>(ser.data), ser.count));
        }
    }
        break;
        case MAVLINK_MSG_ID_AVAILABLE_MODES_MONITOR:
    {
        // Avoid duplicate requests during initial connection setup
        if (!_initialConnectStateMachine || !_initialConnectStateMachine->active()) {
            mavlink_available_modes_monitor_t availableModesMonitor;
            mavlink_msg_available_modes_monitor_decode(&message, &availableModesMonitor);
            _standardModes->availableModesMonitorReceived(availableModesMonitor.seq);
        }
        break;
    }
    case MAVLINK_MSG_ID_CURRENT_MODE:
        _handleCurrentMode(message);
        break;

        // Following are ArduPilot dialect messages
#if !defined(QGC_NO_ARDUPILOT_DIALECT)
    case MAVLINK_MSG_ID_CAMERA_FEEDBACK:
        _handleCameraFeedback(message);
        break;
#endif
    case MAVLINK_MSG_ID_LOG_ENTRY:
    {
        mavlink_log_entry_t log{};
        mavlink_msg_log_entry_decode(&message, &log);
        emit logEntry(log.time_utc, log.size, log.id, log.num_logs, log.last_log_num);
        break;
    }
    case MAVLINK_MSG_ID_LOG_DATA:
    {
        mavlink_log_data_t log{};
        mavlink_msg_log_data_decode(&message, &log);
        if (static_cast<size_t>(log.count) > sizeof(log.data)) {
            qCWarning(VehicleLog) << "Invalid count for LOG_DATA, discarding." << log.count;
        } else {
            emit logData(log.ofs, log.id, log.count, log.data);
        }
        break;
    }
    case MAVLINK_MSG_ID_MESSAGE_INTERVAL:
    {
        _messageIntervalManager->handleMessageInterval(message);
        break;
    }
    case MAVLINK_MSG_ID_CONTROL_STATUS:
        _handleControlStatus(message);
        break;
    case MAVLINK_MSG_ID_COMMAND_LONG:
        _handleCommandLong(message);
        break;
    }

    // This must be emitted after the vehicle processes the message. This way the vehicle state is up to date when anyone else
    // does processing.
    emit mavlinkMessageReceived(message);
}

#if !defined(QGC_NO_ARDUPILOT_DIALECT)
void Vehicle::_handleCameraFeedback(const mavlink_message_t& message)
{
    mavlink_camera_feedback_t feedback;

    mavlink_msg_camera_feedback_decode(&message, &feedback);

    QGeoCoordinate imageCoordinate((double)feedback.lat / qPow(10.0, 7.0), (double)feedback.lng / qPow(10.0, 7.0), feedback.alt_msl);
    qCDebug(VehicleLog) << "_handleCameraFeedback coord:index" << imageCoordinate << feedback.img_idx;
    _cameraTriggerPoints->append(new QGCQGeoCoordinate(imageCoordinate, this));
}
#endif

void Vehicle::_handleOrbitExecutionStatus(const mavlink_message_t& message)
{
    mavlink_orbit_execution_status_t orbitStatus;

    mavlink_msg_orbit_execution_status_decode(&message, &orbitStatus);

    double newRadius =  qAbs(static_cast<double>(orbitStatus.radius));
    if (!QGC::fuzzyCompare(_orbitMapCircle->radius()->rawValue().toDouble(), newRadius)) {
        _orbitMapCircle->radius()->setRawValue(newRadius);
    }

    bool newOrbitClockwise = orbitStatus.radius > 0 ? true : false;
    if (_orbitMapCircle->clockwiseRotation() != newOrbitClockwise) {
        _orbitMapCircle->setClockwiseRotation(newOrbitClockwise);
    }

    QGeoCoordinate newCenter(static_cast<double>(orbitStatus.x) / qPow(10.0, 7.0), static_cast<double>(orbitStatus.y) / qPow(10.0, 7.0));
    if (_orbitMapCircle->center() != newCenter) {
        _orbitMapCircle->setCenter(newCenter);
    }

    if (!_orbitActive) {
        _orbitActive = true;
        _orbitMapCircle->setShowRotation(true);
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
        _cameraTriggerPoints->append(new QGCQGeoCoordinate(imageCoordinate, this));
    }
}

// TODO: VehicleFactGroup
void Vehicle::_handleGpsRawInt(mavlink_message_t& message)
{
    if (message.compid != _defaultComponentId) {
        return;
    }

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

// TODO: VehicleFactGroup
void Vehicle::_handleGlobalPositionInt(mavlink_message_t& message)
{
    if (message.compid != _defaultComponentId) {
        return;
    }

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

// TODO: VehicleFactGroup
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

// TODO: VehicleFactGroup
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
    // ArduPilot has the basemode in the custom0 field of the high latency message.
    if (highLatency2.autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        _base_mode = (uint8_t)highLatency2.custom0;
    } else {
        _base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    }
    _custom_mode = _firmwarePlugin->highLatencyCustomModeTo32Bits(highLatency2.custom_mode);
    if (previousFlightMode != flightMode()) {
        emit flightModeChanged(flightMode());
    }
    // ArduPilot has the arming status (basemode) in the custom0 field of the high latency message.
    if (highLatency2.autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        if ((uint8_t)highLatency2.custom0 & MAV_MODE_FLAG_SAFETY_ARMED && _armed != true) {
            _armed = true;
            emit armedChanged(_armed);
        } else if (!((uint8_t)highLatency2.custom0 & MAV_MODE_FLAG_SAFETY_ARMED) && _armed != false) {
            _armed = false;
            emit armedChanged(_armed);
        }
    } else {
        // Assume armed since we don't know
        if (_armed != true) {
            _armed = true;
            emit armedChanged(_armed);
        }
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

    // Map from MAV_FAILURE bits to standard SYS_STATUS message handling
    const uint32_t newOnboardControlSensorsEnabled = QGCMAVLink::highLatencyFailuresToMavSysStatus(highLatency2);
    if (newOnboardControlSensorsEnabled != _onboardControlSensorsEnabled) {
        _onboardControlSensorsEnabled = newOnboardControlSensorsEnabled;
        _onboardControlSensorsPresent = newOnboardControlSensorsEnabled;
        _onboardControlSensorsUnhealthy = 0;
    }
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
}

QString Vehicle::vehicleUIDStr()
{
    QString uid;
    uint8_t* pUid = (uint8_t*)(void*)&_uid;
    uid = uid.asprintf("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
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
    if (message.compid != _defaultComponentId) {
        return;
    }

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
    return _parameterManager->parameterExists(ParameterManager::defaultComponentId, armingRequireParam) &&
            _parameterManager->getParameter(ParameterManager::defaultComponentId, armingRequireParam)->rawValue().toInt() == 0;
}

void Vehicle::_handleSysStatus(mavlink_message_t& message)
{
    if (message.compid != _defaultComponentId) {
        return;
    }

    mavlink_sys_status_t sysStatus;
    mavlink_msg_sys_status_decode(&message, &sysStatus);

    _sysStatusSensorInfo->update(sysStatus);

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
        emit requiresGpsFixChanged();
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
        if (_batteryFactGroupListModel->count() > 1) {
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
        qCDebug(VehicleLog) << "new home location set at coordinate: " << homeCoord;
        emit homePositionChanged(_homePosition);
    }
}

void Vehicle::_handleHomePosition(mavlink_message_t& message)
{
    if (message.compid != _defaultComponentId) {
        return;
    }

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
            if(SettingsManager::instance()->videoSettings()->disableWhenDisarmed()->rawValue().toBool()) {
                SettingsManager::instance()->videoSettings()->streamEnabled()->setRawValue(false);
                VideoManager::instance()->stopVideo();
            }
        }
    }
}

void Vehicle::_handlePing(LinkInterface* link, mavlink_message_t& message)
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "_handlePing: primary link gone!";
        return;
    }

    mavlink_ping_t      ping;
    mavlink_message_t   msg;

    mavlink_msg_ping_decode(&message, &ping);

    if ((ping.target_system == 0) && (ping.target_component == 0)) {
        // Mavlink defines a ping request as a MSG_ID_PING which contains target_system = 0 and target_component = 0
        // So only send a ping response when you receive a valid ping request
        mavlink_msg_ping_pack_chan(static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
                                   static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
                                   sharedLink->mavlinkChannel(),
                                   &msg,
                                   ping.time_usec,
                                   ping.seq,
                                   message.sysid,
                                   message.compid);
        sendMessageOnLinkThreadSafe(link, msg);
    }
}

void Vehicle::setActuatorsMetadata([[maybe_unused]] uint8_t compid,
                                   const QString &metadataJsonFileName)
{
    if (!_actuators) {
        _actuators = new Actuators(this, this);
    }
    _actuators->load(metadataJsonFileName);
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

void Vehicle::_handleCurrentMode(mavlink_message_t& message)
{
    if (message.compid != _defaultComponentId) {
        return;
    }

    mavlink_current_mode_t currentMode;
    mavlink_msg_current_mode_decode(&message, &currentMode);
    if (currentMode.intended_custom_mode != 0) { // 0 == unknown/not supplied
        _has_custom_mode_user_intention = true;
        QString previousFlightMode = flightMode();
        bool changed = _custom_mode_user_intention != currentMode.intended_custom_mode;
        _custom_mode_user_intention = currentMode.intended_custom_mode;
        if (changed && previousFlightMode != flightMode()) {
            emit flightModeChanged(flightMode());
        }
    }
}

void Vehicle::_handleRCChannels(mavlink_message_t& message)
{
    mavlink_rc_channels_t channels;

    mavlink_msg_rc_channels_decode(&message, &channels);

    QVector<uint16_t> rawChannelValues({
        channels.chan1_raw,
        channels.chan2_raw,
        channels.chan3_raw,
        channels.chan4_raw,
        channels.chan5_raw,
        channels.chan6_raw,
        channels.chan7_raw,
        channels.chan8_raw,
        channels.chan9_raw,
        channels.chan10_raw,
        channels.chan11_raw,
        channels.chan12_raw,
        channels.chan13_raw,
        channels.chan14_raw,
        channels.chan15_raw,
        channels.chan16_raw,
        channels.chan17_raw,
        channels.chan18_raw,
    });

    // The internals of radio calibration can ony deal with contiguous channels (other stuff as well!)
    int validChannelCount = 0;
    int firstUnusedChannelIndex = -1;
    for (int i=0; i<rawChannelValues.size(); i++) {
        if (rawChannelValues[i] != UINT16_MAX) {
            validChannelCount++;
        } else if (firstUnusedChannelIndex == -1) {
            firstUnusedChannelIndex = i;
        }
    }
    if (firstUnusedChannelIndex != -1 && firstUnusedChannelIndex != validChannelCount) {
        qCWarning(VehicleLog) << "Non-contiguous RC channels detected. Not publishing data from RC_CHANNELS.";
        return;
    }

    QVector<int> channelValues(validChannelCount);
    QVector<int> clampedValues(validChannelCount);
    for (int channelIndex = 0; channelIndex < validChannelCount; ++channelIndex) {
        channelValues[channelIndex] = rawChannelValues[channelIndex];
        clampedValues[channelIndex] = std::clamp(channelValues[channelIndex], 1000, 2000);
    }

    // rcRSSI is now a Fact on VehicleFactGroup (this); VehicleFactGroup owns the low-pass
    // filter and sentinel handling for the 0-100 / 255-unknown semantics.
    updateRCRSSI(channels.rssi);
    emit rcChannelsRawChanged(channelValues);
    emit rcChannelsClampedChanged(clampedValues);
}

bool Vehicle::sendMessageOnLinkThreadSafe(LinkInterface* link, mavlink_message_t message)
{
    if (!link->isConnected()) {
        qCDebug(VehicleLog) << "sendMessageOnLinkThreadSafe" << link << "not connected!";
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
    uint8_t frameType = 0;
    if (_vehicleType == MAV_TYPE_SUBMARINE) {
        frameType = parameterManager()->getParameter(_compID, "FRAME_CONFIG")->rawValue().toInt();
    }
    return QGCMAVLink::motorCount(_vehicleType, frameType);
}

bool Vehicle::coaxialMotors()
{
    return _firmwarePlugin->multiRotorCoaxialMotors(this);
}

bool Vehicle::xConfigMotors()
{
    return _firmwarePlugin->multiRotorXConfig(this);
}

void Vehicle::_activeVehicleChanged(Vehicle *newActiveVehicle)
{
    _isActiveVehicle = newActiveVehicle == this;
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
    QStringList flightModes = _firmwarePlugin->flightModes(this);
    return flightModes;
}

QString Vehicle::flightMode() const
{
    return _firmwarePlugin->flightMode(_base_mode, _custom_mode);
}

bool Vehicle::setFlightModeCustom(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    return _firmwarePlugin->setFlightMode(flightMode, base_mode, custom_mode);
}

void Vehicle::setFlightMode(const QString& flightMode)
{
    uint8_t     base_mode;
    uint32_t    custom_mode;

    if (setFlightModeCustom(flightMode, &base_mode, &custom_mode)) {
        SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
        if (!sharedLink) {
            qCDebug(VehicleLog) << "setFlightMode: primary link gone!";
            return;
        }

        uint8_t newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_CUSTOM_MODE;

        // setFlightMode will only set MAV_MODE_FLAG_CUSTOM_MODE_ENABLED in base_mode, we need to move back in the existing
        // states.
        newBaseMode |= base_mode;

        if (_firmwarePlugin->MAV_CMD_DO_SET_MODE_is_supported()) {
            sendMavCommand(defaultComponentId(),
                           MAV_CMD_DO_SET_MODE,
                           true,    // show error if fails
                           MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                           custom_mode);
        } else {
            mavlink_message_t msg;
            mavlink_msg_set_mode_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                           MAVLinkProtocol::getComponentId(),
                                           sharedLink->mavlinkChannel(),
                                           &msg,
                                           id(),
                                           newBaseMode,
                                           custom_mode);
            sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }
    } else {
        qCWarning(VehicleLog) << "FirmwarePlugin::setFlightMode failed, flightMode:" << flightMode;
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
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "requestDataStream: primary link gone!";
        return;
    }

    mavlink_message_t               msg;
    mavlink_request_data_stream_t   dataStream;

    memset(&dataStream, 0, sizeof(dataStream));

    dataStream.req_stream_id = stream;
    dataStream.req_message_rate = rate;
    dataStream.start_stop = 1;  // start
    dataStream.target_system = id();
    dataStream.target_component = _defaultComponentId;

    mavlink_msg_request_data_stream_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                MAVLinkProtocol::getComponentId(),
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

void Vehicle::_sendMessageMultipleNext()
{
    if (_nextSendMessageMultipleIndex < _sendMessageMultipleList.count()) {
        uint32_t msgId = _sendMessageMultipleList[_nextSendMessageMultipleIndex].message.msgid;
        const mavlink_message_info_t* info = mavlink_get_message_info_by_id(msgId);
        QString msgName = info ? info->name : QString::number(msgId);
        qCDebug(VehicleLog) << "_sendMessageMultipleNext:" << msgName;

        SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
        if (sharedLink) {
            sendMessageOnLinkThreadSafe(sharedLink.get(), _sendMessageMultipleList[_nextSendMessageMultipleIndex].message);
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
    QGC::showAppMessage(tr("Mission transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_geoFenceManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    QGC::showAppMessage(tr("GeoFence transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_rallyPointManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    QGC::showAppMessage(tr("Rally Point transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_clearCameraTriggerPoints()
{
    _cameraTriggerPoints->clearAndDeleteContents();
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

void Vehicle::_gotProgressUpdate(float progressValue)
{
    if (sender() != _initialConnectStateMachine && _initialConnectStateMachine->active()) {
        return;
    }
    if (sender() == _initialConnectStateMachine && !_initialConnectStateMachine->active()) {
        progressValue = 0.f;
    }
    _loadProgress = progressValue;
    emit loadProgressChanged(progressValue);
}

void Vehicle::_firstMissionLoadComplete()
{
    disconnect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_firstMissionLoadComplete);
}

void Vehicle::_firstGeoFenceLoadComplete()
{
    disconnect(_geoFenceManager, &GeoFenceManager::loadComplete, this, &Vehicle::_firstGeoFenceLoadComplete);
}

void Vehicle::_firstRallyPointLoadComplete()
{
    disconnect(_rallyPointManager, &RallyPointManager::loadComplete, this, &Vehicle::_firstRallyPointLoadComplete);
    _initialPlanRequestComplete = true;
    emit initialPlanRequestCompleteChanged(true);
}

void Vehicle::_parametersReady(bool parametersReady)
{
    qCDebug(VehicleLog) << "_parametersReady" << parametersReady;

    // Try to set current unix time to the vehicle
    _sendQGCTimeToVehicle();
    // Send time twice, more likely to get to the vehicle on a noisy link
    _sendQGCTimeToVehicle();
    if (parametersReady) {
        disconnect(_parameterManager, &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);
        _setupAutoDisarmSignalling();
    }

    _multirotor_speed_limits_available = _firmwarePlugin->mulirotorSpeedLimitsAvailable(this);
    _fixed_wing_airspeed_limits_available = _firmwarePlugin->fixedWingAirSpeedLimitsAvailable(this);

    emit haveMRSpeedLimChanged();
    emit haveFWSpeedLimChanged();
}

void Vehicle::_sendQGCTimeToVehicle()
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "_sendQGCTimeToVehicle: primary link gone!";
        return;
    }

    mavlink_message_t       msg;
    mavlink_system_time_t   cmd;

    // Timestamp of the master clock in microseconds since UNIX epoch.
    cmd.time_unix_usec = QDateTime::currentDateTime().currentMSecsSinceEpoch()*1000;
    // Timestamp of the component clock since boot time in milliseconds (Not necessary).
    cmd.time_boot_ms = 0;
    mavlink_msg_system_time_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                        MAVLinkProtocol::getComponentId(),
                                        sharedLink->mavlinkChannel(),
                                        &msg,
                                        &cmd);

    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void Vehicle::virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust)
{
    // The following if statement prevents the virtualTabletJoystick from sending values if the standard joystick is enabled
    bool isActiveVehicle = (MultiVehicleManager::instance()->activeVehicle() == this);
    bool joystickEnabled = isActiveVehicle && JoystickManager::instance()->activeJoystickEnabledForActiveVehicle();
    if (!joystickEnabled) {
        sendJoystickDataThreadSafe(
                    static_cast<float>(roll),
                    static_cast<float>(pitch),
                    static_cast<float>(yaw),
                    static_cast<float>(thrust),
                    0, 0, // buttons
                    NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN); // extension values
    }
}

void Vehicle::_say(const QString& text)
{
    AudioOutput::instance()->say(text.toLower());
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

bool Vehicle::spacecraft() const
{
    return QGCMAVLink::isSpacecraft(vehicleType());
}

bool Vehicle::multiRotor() const
{
    return QGCMAVLink::isMultiRotor(vehicleType());
}

bool Vehicle::vtol() const
{
    return QGCMAVLink::isVTOL(vehicleType());
}

QString Vehicle::vehicleTypeString() const
{
    return QGCMAVLink::mavTypeToString(_vehicleType);
}

QString Vehicle::vehicleClassInternalName() const
{
    return QGCMAVLink::vehicleClassToInternalString(vehicleClass());
}

/// Returns the string to speak to identify the vehicle
QString Vehicle::_vehicleIdSpeech()
{
    if (MultiVehicleManager::instance()->vehicles()->count() > 1) {
        return tr("Vehicle %1 ").arg(id());
    } else {
        return QString();
    }
}

void Vehicle::_handleFlightModeChanged(const QString& flightMode)
{
    if (flightMode != _lastAnnouncedFlightMode) {
        _lastAnnouncedFlightMode = flightMode;
        _say(tr("%1 %2 flight mode").arg(_vehicleIdSpeech()).arg(flightMode));
    }
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

QString Vehicle::gotoFlightMode() const
{
    return _firmwarePlugin->gotoFlightMode();
}

void Vehicle::guidedModeRTL(bool smartRTL)
{
    if (!_vehicleSupports->guidedMode()) {
        QGC::showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeRTL(this, smartRTL);
}

void Vehicle::guidedModeLand()
{
    if (!_vehicleSupports->guidedMode()) {
        QGC::showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeLand(this);
}

void Vehicle::guidedModeTakeoff(double altitudeRelative)
{
    if (!_vehicleSupports->guidedMode()) {
        QGC::showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeTakeoff(this, altitudeRelative);
}

double Vehicle::minimumTakeoffAltitudeMeters()
{
    return _firmwarePlugin->minimumTakeoffAltitudeMeters(this);
}

double Vehicle::maximumHorizontalSpeedMultirotorMetersSecond()
{
    return _firmwarePlugin->maximumHorizontalSpeedMultirotorMetersSecond(this);
}


double Vehicle::maximumEquivalentAirspeed()
{
    return _firmwarePlugin->maximumEquivalentAirspeed(this);
}


double Vehicle::minimumEquivalentAirspeed()
{
    return _firmwarePlugin->minimumEquivalentAirspeed(this);
}

bool Vehicle::hasGripper()  const
{
    return _firmwarePlugin->hasGripper(this);
}

void Vehicle::startTakeoff()
{
    _firmwarePlugin->startTakeoff(this);
}


void Vehicle::startMission()
{
    _firmwarePlugin->startMission(this);
}

bool Vehicle::guidedModeGotoLocation(const QGeoCoordinate& gotoCoord, double forwardFlightLoiterRadius)
{
    if (!_vehicleSupports->guidedMode()) {
        QGC::showAppMessage(guided_mode_not_supported_by_vehicle);
        return false;
    }
    if (!coordinate().isValid()) {
        return false;
    }
    if (!gotoCoord.isValid()) {
        return false;
    }
    double maxDistance = SettingsManager::instance()->flyViewSettings()->maxGoToLocationDistance()->rawValue().toDouble();
    if (coordinate().distanceTo(gotoCoord) > maxDistance) {
        QGC::showAppMessage(QString("New location is too far. Must be less than %1 %2.").arg(qRound(FactMetaData::metersToAppSettingsHorizontalDistanceUnits(maxDistance).toDouble())).arg(FactMetaData::appSettingsHorizontalDistanceUnitsString()));
        return false;
    }

    return _firmwarePlugin->guidedModeGotoLocation(this, gotoCoord, forwardFlightLoiterRadius);
}

void Vehicle::guidedModeChangeAltitude(double altitudeChange, bool pauseVehicle)
{
    if (!_vehicleSupports->guidedMode()) {
        QGC::showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeAltitude(this, altitudeChange, pauseVehicle);
}

void
Vehicle::guidedModeChangeGroundSpeedMetersSecond(double groundspeed)
{
    if (!_vehicleSupports->guidedMode()) {
        QGC::showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeGroundSpeedMetersSecond(this, groundspeed);
}

void
Vehicle::guidedModeChangeEquivalentAirspeedMetersSecond(double airspeed)
{
    if (!_vehicleSupports->guidedMode()) {
        QGC::showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeEquivalentAirspeedMetersSecond(this, airspeed);
}

void Vehicle::guidedModeOrbit(const QGeoCoordinate& centerCoord, double radius, double amslAltitude)
{
    if (!_vehicleSupports->orbitMode()) {
        QGC::showAppMessage(QStringLiteral("Orbit mode not supported by Vehicle."));
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
                    static_cast<float>(ORBIT_YAW_BEHAVIOUR_UNCHANGED),       // Use current or vehicle default yaw behavior
                    static_cast<float>(qQNaN()),    // Use vehicle default num of orbits behavior
                    centerCoord.latitude(), centerCoord.longitude(), static_cast<float>(amslAltitude));
    } else {
        sendMavCommand(
                    defaultComponentId(),
                    MAV_CMD_DO_ORBIT,
                    true,                           // show error if fails
                    static_cast<float>(radius),
                    static_cast<float>(qQNaN()),    // Use default velocity
                    static_cast<float>(ORBIT_YAW_BEHAVIOUR_UNCHANGED),       // Use current or vehicle default yaw behavior
                    static_cast<float>(qQNaN()),    // Use vehicle default num of orbits behavior
                    static_cast<float>(centerCoord.latitude()),
                    static_cast<float>(centerCoord.longitude()),
                    static_cast<float>(amslAltitude));
    }
}

void Vehicle::guidedModeROI(const QGeoCoordinate& centerCoord)
{
    if (!centerCoord.isValid()) {
        return;
    }
    if (!_vehicleSupports->roiMode()) {
        QGC::showAppMessage(QStringLiteral("ROI mode not supported by Vehicle."));
        return;
    }

    if (px4Firmware()) {
        // PX4 ignores the coordinate frame in COMMAND_INT and treats the altitude as AMSL,
        // so a terrain query is required before we can send the ROI command.
        _terrainQueryCoordinator->roiWithTerrain(centerCoord);
    } else {
        // ArduPilot handles MAV_FRAME_GLOBAL_RELATIVE_ALT correctly, so altitude 0 relative to
        // home is a reasonable default for a map click with no altitude info.
        // Sanity check Ardupilot. Max altitude processed is 83000
        if ((centerCoord.altitude() >= 83000) || (centerCoord.altitude() <= -83000)) {
            return;
        }
        _terrainQueryCoordinator->sendROICommand(centerCoord, MAV_FRAME_GLOBAL_RELATIVE_ALT, static_cast<float>(centerCoord.altitude()));
    }

    // This is picked by qml to display coordinate over map
    emit roiCoordChanged(centerCoord);
}

void Vehicle::stopGuidedModeROI()
{
    if (!_vehicleSupports->roiMode()) {
        QGC::showAppMessage(QStringLiteral("ROI mode not supported by Vehicle."));
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

void Vehicle::guidedModeChangeHeading(const QGeoCoordinate &headingCoord)
{
    if (!_vehicleSupports->changeHeading()) {
        QGC::showAppMessage(tr("Change Heading not supported by Vehicle."));
        return;
    }

    _firmwarePlugin->guidedModeChangeHeading(this, headingCoord);
}

void Vehicle::pauseVehicle()
{
    if (!_vehicleSupports->pauseVehicle()) {
        QGC::showAppMessage(QStringLiteral("Pause not supported by vehicle."));
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

bool Vehicle::inFwdFlight() const
{
    return fixedWing() || _vtolInFwdFlight;
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

void Vehicle::landingGearDeploy()
{
    sendMavCommand(
                defaultComponentId(),
                MAV_CMD_AIRFRAME_CONFIGURATION,
                true,       // show error if fails
                -1.0f,      // all gears
                0.0f);      // down
}

void Vehicle::landingGearRetract()
{
    sendMavCommand(
                defaultComponentId(),
                MAV_CMD_AIRFRAME_CONFIGURATION,
                true,       // show error if fails
                -1.0f,      // all gears
                1.0f);      // up
}

void Vehicle::setCurrentMissionSequence(int seq)
{
    if (!_firmwarePlugin->sendHomePositionToVehicle()) {
        seq--;
    }

    // send the mavlink command (created in Jan 2019)
    sendMavCommandWithLambdaFallback(
        [this,seq]() {  // lambda function which uses the deprecated mission_set_current
            SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
            if (!sharedLink) {
                qCDebug(VehicleLog) << "setCurrentMissionSequence: primary link gone!";
                return;
            }

            mavlink_message_t       msg;

            // send mavlink message (deprecated since Aug 2022).
            mavlink_msg_mission_set_current_pack_chan(
                static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
                static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
                sharedLink->mavlinkChannel(),
                &msg,
                static_cast<uint8_t>(id()),
                _compID,
                static_cast<uint16_t>(seq));
            sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        },
        static_cast<uint8_t>(defaultComponentId()),
        MAV_CMD_DO_SET_MISSION_CURRENT,
        true, // showError
        static_cast<uint16_t>(seq)
    );
}

void Vehicle::sendMavCommand(int compId, MAV_CMD command, bool showError, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _mavCmdQueue->sendCommand(compId, command, showError, param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandDelayed(int compId, MAV_CMD command, bool showError, int milliseconds, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _mavCmdQueue->sendCommandDelayed(compId, command, showError, milliseconds, param1, param2, param3, param4, param5, param6, param7);
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

void Vehicle::sendMavCommandWithHandler(const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _mavCmdQueue->sendCommandWithHandler(ackHandlerInfo, compId, command, param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandInt(int compId, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    _mavCmdQueue->sendCommandInt(compId, command, frame, showError, param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandIntWithHandler(const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, MAV_FRAME frame, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    _mavCmdQueue->sendCommandIntWithHandler(ackHandlerInfo, compId, command, frame, param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandWithLambdaFallback(std::function<void()> lambda, int compId, MAV_CMD command, bool showError, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _mavCmdQueue->sendCommandWithLambdaFallback(std::move(lambda), compId, command, showError, param1, param2, param3, param4, param5, param6, param7);
}

bool Vehicle::isMavCommandPending(int targetCompId, MAV_CMD command)
{
    return _mavCmdQueue->isPending(targetCompId, command);
}

int Vehicle::_findMavCommandListEntryIndex(int targetCompId, MAV_CMD command)
{
    return _mavCmdQueue->findEntryIndex(targetCompId, command);
}

void Vehicle::showCommandAckError(const mavlink_command_ack_t& ack)
{
    MavCommandQueue::showCommandAckError(ack);
}

void Vehicle::_handleCommandAck(mavlink_message_t& message)
{
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&message, &ack);

    QString rawCommandName = MissionCommandTree::instance()->rawName(static_cast<MAV_CMD>(ack.command));
    QString logMsg = QStringLiteral("_handleCommandAck command(%1) result(%2)").arg(rawCommandName).arg(QGCMAVLink::mavResultToString(static_cast<MAV_RESULT>(ack.result)));

    // For REQUEST_MESSAGE commands, also log which message was requested.
    if (ack.command == MAV_CMD_REQUEST_MESSAGE) {
        const int entryIndex = _mavCmdQueue->findEntryIndex(message.compid, static_cast<MAV_CMD>(ack.command));
        if (entryIndex != -1) {
            // The message id was sent as param1 of MAV_CMD_REQUEST_MESSAGE. We can't read it back
            // from the queue without exposing entry internals, so just log the ack summary.
            logMsg += QStringLiteral(" (entry=%1)").arg(entryIndex);
        }
    }

    qCDebug(VehicleLog) << logMsg;

    // Vehicle-level side effects that must fire regardless of queue-match state.
    if (ack.command == MAV_CMD_DO_SET_ROI_LOCATION && ack.result == MAV_RESULT_ACCEPTED) {
        _isROIEnabled = true;
        emit isROIEnabledChanged();
    }
    if (ack.command == MAV_CMD_DO_SET_ROI_NONE && ack.result == MAV_RESULT_ACCEPTED) {
        _isROIEnabled = false;
        emit isROIEnabledChanged();
    }
    if (ack.command == MAV_CMD_PREFLIGHT_STORAGE) {
        emit sensorsParametersResetAck(ack.result == MAV_RESULT_ACCEPTED);
    }
#if !defined(QGC_NO_ARDUPILOT_DIALECT)
    if (ack.command == MAV_CMD_FLASH_BOOTLOADER && ack.result == MAV_RESULT_ACCEPTED) {
        QGC::showAppMessage(tr("Bootloader flash succeeded"));
    }
#endif

    // Delegate queue-matching + user callbacks to MavCommandQueue.
    _mavCmdQueue->handleCommandAck(message, ack);

    // Advance PID tuning setup/teardown.
    if (ack.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        _mavlinkStreamConfig->gotSetMessageIntervalAck();
    }
}

void Vehicle::requestMessage(RequestMessageResultHandler resultHandler, void* resultHandlerData, int compId, int messageId, float param1, float param2, float param3, float param4, float param5)
{
    _reqMsgCoord->requestMessage(resultHandler, resultHandlerData, compId, messageId, param1, param2, param3, param4, param5);
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
    return QGCMAVLink::firmwareVersionTypeToString(_firmwareVersionType);
}

void Vehicle::_rebootCommandResultHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    Vehicle* vehicle = static_cast<Vehicle*>(resultHandlerData);

    if (ack.result != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
        case MavCmdResultCommandResultOnly:
            qCDebug(VehicleLog) << QStringLiteral("MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN error(%1)").arg(ack.result);
            break;
        case MavCmdResultFailureNoResponseToCommand:
            qCDebug(VehicleLog) << "MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN failed: no response from vehicle";
            break;
        case MavCmdResultFailureDuplicateCommand:
            qCDebug(VehicleLog) << "MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN failed: duplicate command";
            break;
        }
        QGC::showAppMessage(tr("Vehicle reboot failed."));
    } else {
        vehicle->closeVehicle();
    }
}

void Vehicle::rebootVehicle()
{
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler       = _rebootCommandResultHandler;
    handlerInfo.resultHandlerData   = this;

    sendMavCommandWithHandler(&handlerInfo, _defaultComponentId, MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1);
}

void Vehicle::startCalibration(QGCMAVLink::CalibrationType calType)
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "startCalibration: primary link gone!";
        return;
    }

    float param1 = 0;
    float param2 = 0;
    float param3 = 0;
    float param4 = 0;
    float param5 = 0;
    float param6 = 0;
    float param7 = 0;

    switch (calType) {
    case QGCMAVLink::CalibrationGyro:
        param1 = 1;
        break;
    case QGCMAVLink::CalibrationMag:
        param2 = 1;
        break;
    case QGCMAVLink::CalibrationRadio:
        param4 = 1;
        break;
    case QGCMAVLink::CalibrationCopyTrims:
        param4 = 2;
        break;
    case QGCMAVLink::CalibrationAccel:
        param5 = 1;
        break;
    case QGCMAVLink::CalibrationLevel:
        param5 = 2;
        break;
    case QGCMAVLink::CalibrationEsc:
        param7 = 1;
        break;
    case QGCMAVLink::CalibrationPX4Airspeed:
        param6 = 1;
        break;
    case QGCMAVLink::CalibrationPX4Pressure:
        param3 = 1;
        break;
    case QGCMAVLink::CalibrationAPMCompassMot:
        param6 = 1;
        break;
    case QGCMAVLink::CalibrationAPMPressureAirspeed:
        param3 = 1;
        break;
    case QGCMAVLink::CalibrationAPMPreFlight:
        param3 = 1; // GroundPressure/Airspeed
        if (multiRotor() || rover()) {
            // Gyro cal for ArduCopter only
            param1 = 1;
        }
        break;
    case QGCMAVLink::CalibrationAPMAccelSimple:
        param5 = 4;
        break;
    case QGCMAVLink::CalibrationNone:
    default:
        break;
    }

    // We can't use sendMavCommand here since we have no idea how long it will be before the command returns a result. This in turn
    // causes the retry logic to break down.
    mavlink_message_t msg;
    mavlink_msg_command_long_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                       MAVLinkProtocol::getComponentId(),
                                       sharedLink->mavlinkChannel(),
                                       &msg,
                                       id(),
                                       defaultComponentId(),            // target component
                                       MAV_CMD_PREFLIGHT_CALIBRATION,    // command id
                                       0,                                // 0=first transmission of command
                                       param1, param2, param3, param4, param5, param6, param7);
    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void Vehicle::stopCalibration(bool showError)
{
    sendMavCommand(defaultComponentId(),    // target component
                   MAV_CMD_PREFLIGHT_CALIBRATION,     // command id
                   showError,
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

void Vehicle::setOfflineEditingDefaultComponentId(int defaultComponentId)
{
    if (_offlineEditingVehicle) {
        _defaultComponentId = defaultComponentId;
    } else {
        qCWarning(VehicleLog) << "Call to Vehicle::setOfflineEditingDefaultComponentId on vehicle which is not offline";
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
    SharedLinkInterfacePtr  sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "_ackMavlinkLogData: primary link gone!";
        return;
    }

    mavlink_message_t       msg;
    mavlink_logging_ack_t   ack;

    memset(&ack, 0, sizeof(ack));
    ack.sequence = sequence;
    ack.target_component = _defaultComponentId;
    ack.target_system = id();
    mavlink_msg_logging_ack_encode_chan(
                MAVLinkProtocol::instance()->getSystemId(),
                MAVLinkProtocol::getComponentId(),
                sharedLink->mavlinkChannel(),
                &msg,
                &ack);
    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void Vehicle::_handleMavlinkLoggingData(mavlink_message_t& message)
{
    mavlink_logging_data_t log;
    mavlink_msg_logging_data_decode(&message, &log);
    if (static_cast<size_t>(log.length) > sizeof(log.data)) {
        qWarning() << "Invalid length for LOGGING_DATA, discarding." << log.length;
    } else {
        emit mavlinkLogData(this, log.target_system, log.target_component, log.sequence,
                            log.first_message_offset, QByteArray((const char*)log.data, log.length), false);
    }
}

void Vehicle::_handleMavlinkLoggingDataAcked(mavlink_message_t& message)
{
    mavlink_logging_data_acked_t log;
    mavlink_msg_logging_data_acked_decode(&message, &log);
    _ackMavlinkLogData(log.sequence);
    if (static_cast<size_t>(log.length) > sizeof(log.data)) {
        qWarning() << "Invalid length for LOGGING_DATA_ACKED, discarding." << log.length;
    } else {
        emit mavlinkLogData(this, log.target_system, log.target_component, log.sequence,
                            log.first_message_offset, QByteArray((const char*)log.data, log.length), false);
    }
}

void Vehicle::setFirmwarePluginInstanceData(FirmwarePluginInstanceData* firmwarePluginInstanceData)
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

QString Vehicle::motorDetectionFlightMode() const
{
    return _firmwarePlugin->motorDetectionFlightMode();
}

QString Vehicle::stabilizedFlightMode() const
{
    return _firmwarePlugin->stabilizedFlightMode();
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

const QVariantList& Vehicle::toolIndicators()
{
    if(_firmwarePlugin) {
        return _firmwarePlugin->toolIndicators(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

void Vehicle::_setupAutoDisarmSignalling()
{
    QString param = _firmwarePlugin->autoDisarmParameter(this);

    if (!param.isEmpty() && _parameterManager->parameterExists(ParameterManager::defaultComponentId, param)) {
        Fact* fact = _parameterManager->getParameter(ParameterManager::defaultComponentId,param);
        connect(fact, &Fact::rawValueChanged, this, &Vehicle::autoDisarmChanged);
        emit autoDisarmChanged();
    }
}

bool Vehicle::autoDisarm()
{
    QString param = _firmwarePlugin->autoDisarmParameter(this);

    if (!param.isEmpty() && _parameterManager->parameterExists(ParameterManager::defaultComponentId, param)) {
        Fact* fact = _parameterManager->getParameter(ParameterManager::defaultComponentId,param);
        return fact->rawValue().toDouble() > 0;
    }

    return false;
}

void Vehicle::_updateDistanceHeadingHome()
{
    if (coordinate().isValid() && homePosition().isValid()) {
        _distanceToHomeFact.setRawValue(coordinate().distanceTo(homePosition()));
        if (_distanceToHomeFact.rawValue().toDouble() > 1.0) {
            _headingToHomeFact.setRawValue(coordinate().azimuthTo(homePosition()));
            _headingFromHomeFact.setRawValue(homePosition().azimuthTo(coordinate()));
        } else {
            _headingToHomeFact.setRawValue(qQNaN());
            _headingFromHomeFact.setRawValue(qQNaN());
        }
    } else {
        _distanceToHomeFact.setRawValue(qQNaN());
        _headingToHomeFact.setRawValue(qQNaN());
        _headingFromHomeFact.setRawValue(qQNaN());
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

void Vehicle::_updateDistanceHeadingGCS()
{
    QGeoCoordinate gcsPosition = QGCPositionManager::instance()->gcsPosition();
    if (coordinate().isValid() && gcsPosition.isValid()) {
        _distanceToGCSFact.setRawValue(coordinate().distanceTo(gcsPosition));
        _headingFromGCSFact.setRawValue(gcsPosition.azimuthTo(coordinate()));
    } else {
        _distanceToGCSFact.setRawValue(qQNaN());
        _headingFromGCSFact.setRawValue(qQNaN());
    }
}

void Vehicle::_updateHomepoint()
{
    const bool setHomeCmdSupported = firmwarePlugin()->supportedMissionCommands(vehicleClass()).contains(MAV_CMD_DO_SET_HOME);
    const bool updateHomeActivated = SettingsManager::instance()->flyViewSettings()->updateHomePosition()->rawValue().toBool();
    if(setHomeCmdSupported && updateHomeActivated){
        QGeoCoordinate gcsPosition = QGCPositionManager::instance()->gcsPosition();
        if (coordinate().isValid() && gcsPosition.isValid()) {
            sendMavCommand(defaultComponentId(),
                           MAV_CMD_DO_SET_HOME, false,
                           0,
                           0, 0, 0,
                           static_cast<float>(gcsPosition.latitude()) ,
                           static_cast<float>(gcsPosition.longitude()),
                           static_cast<float>(gcsPosition.altitude()));
        }
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
    return _firmwarePlugin->getHobbsMeter(this);
}

void Vehicle::_vehicleParamLoaded(bool ready)
{
    //-- TODO: This seems silly but can you think of a better
    //   way to update this?
    if(ready) {
        emit hobbsMeterChanged();
    }
}

void Vehicle::_mavlinkMessageStatus(int uasId, uint64_t totalSent, uint64_t totalReceived, uint64_t totalLoss, float lossPercent)
{
    if(uasId == _systemID) {
        _mavlinkSentCount       = totalSent;
        _mavlinkReceivedCount   = totalReceived;
        _mavlinkLossCount       = totalLoss;
        _mavlinkLossPercent     = lossPercent;
        emit mavlinkStatusChanged();

        // Update signing status from the primary link's channel
        bool signing = false;
        SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
        if (sharedLink) {
            signing = MAVLinkSigning::isSigningEnabled(static_cast<mavlink_channel_t>(sharedLink->mavlinkChannel()));
        }
        if (signing != _mavlinkSigning) {
            _mavlinkSigning = signing;
            if (!signing) {
                _mavlinkSigningKeyName.clear();
            }
            emit mavlinkSigningChanged();
        }
    }
}

int Vehicle::versionCompare(const QString& compare) const
{
    return _firmwarePlugin->versionCompare(this, compare);
}

int Vehicle::versionCompare(int major, int minor, int patch) const
{
    return _firmwarePlugin->versionCompare(this, major, minor, patch);
}

void Vehicle::setPIDTuningTelemetryMode(PIDTuningTelemetryMode mode)
{
    bool liveUpdate = mode != ModeDisabled;
    setLiveUpdates(liveUpdate);
    _setpointFactGroup->setLiveUpdates(liveUpdate);
    _localPositionFactGroup->setLiveUpdates(liveUpdate);
    _localPositionSetpointFactGroup->setLiveUpdates(liveUpdate);

    switch (mode) {
    case ModeDisabled:
        _mavlinkStreamConfig->restoreDefaults();
        break;
    case ModeRateAndAttitude:
        _mavlinkStreamConfig->setHighRateRateAndAttitude();
        break;
    case ModeVelocityAndPosition:
        _mavlinkStreamConfig->setHighRateVelAndPos();
        break;
    case ModeAltitudeAndAirspeed:
        _mavlinkStreamConfig->setHighRateAltAirspeed();
        // reset the altitude offset to the current value, so the plotted value is around 0
        if (!qIsNaN(_altitudeTuningOffset)) {
            _altitudeTuningOffset += _altitudeTuningFact.rawValue().toDouble();
            _altitudeTuningSetpointFact.setRawValue(0.f);
            _altitudeTuningFact.setRawValue(0.f);
        }
        break;
    }
}

void Vehicle::_setMessageInterval(int messageId, int rate)
{
    sendMavCommand(defaultComponentId(),
                   MAV_CMD_SET_MESSAGE_INTERVAL,
                   true,                        // show error
                   messageId,
                   rate);
}

QString Vehicle::_formatMavCommand(MAV_CMD command, float param1)
{
    QString commandName = MissionCommandTree::instance()->rawName(command);

    if (command == MAV_CMD_REQUEST_MESSAGE && param1 > 0) {
        const mavlink_message_info_t* info = mavlink_get_message_info_by_id(static_cast<uint32_t>(param1));
        QString param1Str = info ? QString("%1(%2)").arg(param1).arg(info->name) : QString::number(param1);
        return QString("%1: %2").arg(commandName).arg(param1Str);
    }
    return QString("%1: %2").arg(commandName).arg(param1);
}

bool Vehicle::isInitialConnectComplete() const
{
    return !_initialConnectStateMachine->active();
}

void Vehicle::_initializeCsv()
{
    if (!SettingsManager::instance()->mavlinkSettings()->saveCsvTelemetry()->rawValue().toBool()) {
        return;
    }
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    QString fileName = QString("%1 vehicle%2.csv").arg(now).arg(_systemID);
    QDir saveDir(SettingsManager::instance()->appSettings()->telemetrySavePath());
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
            (_armed || SettingsManager::instance()->mavlinkSettings()->telemetrySaveNotArmed()->rawValue().toBool())){
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

void Vehicle::doSetHome(const QGeoCoordinate& coord)
{
    _terrainQueryCoordinator->doSetHomeWithTerrain(coord);
}

void Vehicle::_handleObstacleDistance(const mavlink_message_t& message)
{
    mavlink_obstacle_distance_t o;
    mavlink_msg_obstacle_distance_decode(&message, &o);
    _objectAvoidance->update(&o);
}

void Vehicle::_handleFenceStatus(const mavlink_message_t& message)
{
    mavlink_fence_status_t fenceStatus;

    mavlink_msg_fence_status_decode(&message, &fenceStatus);

    qCDebug(VehicleLog) << "_handleFenceStatus breach_status" << fenceStatus.breach_status;

    static qint64 lastUpdate = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (fenceStatus.breach_status == 1) {
        if (now - lastUpdate > 3000) {
            lastUpdate = now;
            QString breachTypeStr;
            switch (fenceStatus.breach_type) {
                case FENCE_BREACH_NONE:
                    return;
                case FENCE_BREACH_MINALT:
                    breachTypeStr = tr("minimum altitude");
                    break;
                case FENCE_BREACH_MAXALT:
                    breachTypeStr = tr("maximum altitude");
                    break;
                case FENCE_BREACH_BOUNDARY:
                    breachTypeStr = tr("boundary");
                    break;
                default:
                    break;
            }

            _say(breachTypeStr + " " + tr("fence breached"));
        }
    } else {
        lastUpdate = now;
    }
}

void Vehicle::updateFlightDistance(double distance)
{
    _flightDistanceFact.setRawValue(_flightDistanceFact.rawValue().toDouble() + distance);
}

void Vehicle::sendParamMapRC(const QString& paramName, double scale, double centerValue, int tuningID, double minValue, double maxValue)
{
    SharedLinkInterfacePtr  sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "sendParamMapRC: primary link gone!";
        return;
    }

    mavlink_message_t       message;

    char param_id_cstr[MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN] = {};
    // Copy string into buffer, ensuring not to exceed the buffer size
    for (unsigned int i = 0; i < sizeof(param_id_cstr); i++) {
        if ((int)i < paramName.length()) {
            param_id_cstr[i] = paramName.toLatin1()[i];
        }
    }

    mavlink_msg_param_map_rc_pack_chan(static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
                                       static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
                                       sharedLink->mavlinkChannel(),
                                       &message,
                                       _systemID,
                                       MAV_COMP_ID_AUTOPILOT1,
                                       param_id_cstr,
                                       -1,                                                  // parameter name specified as string in previous argument
                                       static_cast<uint8_t>(tuningID),
                                       static_cast<float>(centerValue),
                                       static_cast<float>(scale),
                                       static_cast<float>(minValue),
                                       static_cast<float>(maxValue));
    sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

void Vehicle::clearAllParamMapRC(void)
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog)<< "clearAllParamMapRC: primary link gone!";
        return;
    }

    char param_id_cstr[MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN] = {};

    for (int i = 0; i < 3; i++) {
        mavlink_message_t message;
        mavlink_msg_param_map_rc_pack_chan(static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
                                           static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
                                           sharedLink->mavlinkChannel(),
                                           &message,
                                           _systemID,
                                           MAV_COMP_ID_AUTOPILOT1,
                                           param_id_cstr,
                                           -2,                                                  // Disable map for specified tuning id
                                           i,                                                   // tuning id
                                           0, 0, 0, 0);                                         // unused
        sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }
}

void Vehicle::sendJoystickDataThreadSafe(float roll, float pitch, float yaw, float thrust, quint16 buttons, quint16 buttons2, float pitchExtension, float rollExtension, float aux1, float aux2, float aux3, float aux4, float aux5, float aux6)
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog)<< "sendJoystickDataThreadSafe: primary link gone!";
        return;
    }

    if (sharedLink->linkConfiguration()->isHighLatency()) {
        return;
    }

    mavlink_message_t message;

    float axesScaling = 1.0 * 1000.0;
    uint8_t extensions = 0;

    // Incoming values are in the range -1:1
    float newRollCommand =      roll * axesScaling;
    float newPitchCommand  =    pitch * axesScaling;
    float newYawCommand    =    yaw * axesScaling;
    float newThrustCommand =    thrust * axesScaling;

    // Scale and set extension bits/values
    float incomingExtensionValues[] = { pitchExtension, rollExtension, aux1, aux2, aux3, aux4, aux5, aux6 };
    int16_t outgoingExtensionValues[std::size(incomingExtensionValues)];
    for (size_t i = 0; i < std::size(incomingExtensionValues); i++) {
        int16_t scaledValue = 0;
        if (!qIsNaN(incomingExtensionValues[i])) {
            scaledValue = static_cast<int16_t>(incomingExtensionValues[i] * axesScaling);
            extensions |= (1 << i);
        }
        outgoingExtensionValues[i] = scaledValue;
    }
    mavlink_msg_manual_control_pack_chan(
        static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
        static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
        sharedLink->mavlinkChannel(),
        &message,
        static_cast<uint8_t>(_systemID),
        static_cast<int16_t>(newPitchCommand),
        static_cast<int16_t>(newRollCommand),
        static_cast<int16_t>(newThrustCommand),
        static_cast<int16_t>(newYawCommand),
        buttons, buttons2,
        extensions,
        outgoingExtensionValues[0],
        outgoingExtensionValues[1],
        outgoingExtensionValues[2],
        outgoingExtensionValues[3],
        outgoingExtensionValues[4],
        outgoingExtensionValues[5],
        outgoingExtensionValues[6],
        outgoingExtensionValues[7]
    );
    sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

void Vehicle::triggerSimpleCamera()
{
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_DO_DIGICAM_CONTROL,
                   true,                        // show errors
                   0.0, 0.0, 0.0, 0.0,          // param 1-4 unused
                   1.0);                        // trigger camera
}

void Vehicle::sendGripperAction(GRIPPER_ACTIONS gripperAction)
{
    sendMavCommand(
            _defaultComponentId,
            MAV_CMD_DO_GRIPPER,
            true,                   // Show errors
            0,                      // Param1: Gripper ID (Always set to 0)
            gripperAction);         // Param2: Gripper Action
}

void Vehicle::setEstimatorOrigin(const QGeoCoordinate& centerCoord)
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "setEstimatorOrigin: primary link gone!";
        return;
    }

    mavlink_message_t msg;
    mavlink_msg_set_gps_global_origin_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        id(),
        centerCoord.latitude() * 1e7,
        centerCoord.longitude() * 1e7,
        centerCoord.altitude() * 1e3,
        static_cast<float>(qQNaN())
    );
    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void Vehicle::pairRX(int rxType, int rxSubType)
{
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_START_RX_PAIR,
                   true,
                   rxType,
                   rxSubType);
}

void Vehicle::startTimerRevertAllowTakeover()
{
    _timerRevertAllowTakeover.stop();
    _timerRevertAllowTakeover.setSingleShot(true);
    _timerRevertAllowTakeover.setInterval(operatorControlTakeoverTimeoutMsecs());
    // Disconnect any previous connections to avoid multiple handlers
    disconnect(&_timerRevertAllowTakeover, &QTimer::timeout, nullptr, nullptr);

    connect(&_timerRevertAllowTakeover, &QTimer::timeout, this, [this](){
        if (MAVLinkProtocol::instance()->getSystemId() == _sysid_in_control) {
            this->requestOperatorControl(false);
        }
    });
    _timerRevertAllowTakeover.start();
}

void Vehicle::requestOperatorControl(bool allowOverride, int requestTimeoutSecs)
{
    int safeRequestTimeoutSecs;
    int requestTimeoutSecsMin = SettingsManager::instance()->flyViewSettings()->requestControlTimeout()->cookedMin().toInt();
    int requestTimeoutSecsMax = SettingsManager::instance()->flyViewSettings()->requestControlTimeout()->cookedMax().toInt();
    if (requestTimeoutSecs >= requestTimeoutSecsMin && requestTimeoutSecs <= requestTimeoutSecsMax) {
        safeRequestTimeoutSecs = requestTimeoutSecs;
    } else {
        // If out of limits use default value
        safeRequestTimeoutSecs = SettingsManager::instance()->flyViewSettings()->requestControlTimeout()->cookedDefaultValue().toInt();
    }

    const MavCmdAckHandlerInfo_t handlerInfo = {&Vehicle::_requestOperatorControlAckHandler, this, nullptr, nullptr};
    sendMavCommandWithHandler(
        &handlerInfo,
        _defaultComponentId,
        MAV_CMD_REQUEST_OPERATOR_CONTROL,
        0,                                  // System ID of GCS requesting control, 0 if it is this GCS
        1,                                  // Action - 0: Release control, 1: Request control.
        allowOverride ? 1 : 0,              // Allow takeover - Enable automatic granting of ownership on request. 0: Ask current owner and reject request, 1: Allow automatic takeover.
        safeRequestTimeoutSecs              // Timeout in seconds before a request to a GCS to allow takeover is assumed to be rejected. This is used to display the timeout graphically on requestor and GCS in control.
    );

    // If this is a request we sent to other GCS, start timer so User can not keep sending requests until the current timeout expires
    if (requestTimeoutSecs > 0) {
        requestOperatorControlStartTimer(requestTimeoutSecs * 1000);
    }
}

void Vehicle::_requestOperatorControlAckHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    // For the moment, this will always come from an autopilot, compid 1
    Q_UNUSED(compId);

    // If duplicated or no response, show popup to user. Otherwise only log it.
    switch (failureCode) {
        case MavCmdResultFailureDuplicateCommand:
            QGC::showAppMessage(tr("Waiting for previous operator control request"));
            return;
        case MavCmdResultFailureNoResponseToCommand:
            QGC::showAppMessage(tr("No response to operator control request"));
            return;
        default:
            break;
    }

    Vehicle* vehicle = static_cast<Vehicle*>(resultHandlerData);
    if (!vehicle) {
        return;
    }

    if (ack.result == MAV_RESULT_ACCEPTED) {
        qCDebug(VehicleLog) << "Operator control request accepted";
    } else {
        qCDebug(VehicleLog) << "Operator control request rejected";
    }
}

void Vehicle::requestOperatorControlStartTimer(int requestTimeoutMsecs)
{
    // First flag requests not allowed
    _sendControlRequestAllowed = false;
    emit sendControlRequestAllowedChanged(false);
    // Setup timer to re enable it again after timeout
    _timerRequestOperatorControl.stop();
    _timerRequestOperatorControl.setSingleShot(true);
    _timerRequestOperatorControl.setInterval(requestTimeoutMsecs);
    // Disconnect any previous connections to avoid multiple handlers
    disconnect(&_timerRequestOperatorControl, &QTimer::timeout, nullptr, nullptr);
    connect(&_timerRequestOperatorControl, &QTimer::timeout, this, [this](){
        _sendControlRequestAllowed = true;
        emit sendControlRequestAllowedChanged(true);
    });
    _timerRequestOperatorControl.start();
}

void Vehicle::_handleControlStatus(const mavlink_message_t& message)
{
    mavlink_control_status_t controlStatus;
    mavlink_msg_control_status_decode(&message, &controlStatus);

    bool updateControlStatusSignals = false;
    if (_gcsControlStatusFlags != controlStatus.flags) {
        _gcsControlStatusFlags = controlStatus.flags;
        _gcsControlStatusFlags_SystemManager = controlStatus.flags & GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER;
        _gcsControlStatusFlags_TakeoverAllowed = controlStatus.flags & GCS_CONTROL_STATUS_FLAGS_TAKEOVER_ALLOWED;
        updateControlStatusSignals = true;
    }

    if (_sysid_in_control != controlStatus.sysid_in_control) {
        _sysid_in_control = controlStatus.sysid_in_control;
        updateControlStatusSignals = true;
    }

    if (!_firstControlStatusReceived) {
        _firstControlStatusReceived = true;
        updateControlStatusSignals = true;
    }

    if (updateControlStatusSignals) {
        emit gcsControlStatusChanged();
    }

    // If we were waiting for a request to be accepted and now it was accepted, adjust flags accordingly so
    // UI unlocks the request/take control button
    if (!sendControlRequestAllowed() && _gcsControlStatusFlags_TakeoverAllowed) {
        disconnect(&_timerRequestOperatorControl, &QTimer::timeout, nullptr, nullptr);
        _sendControlRequestAllowed = true;
        emit sendControlRequestAllowedChanged(true);
    }
}

void Vehicle::_handleCommandRequestOperatorControl(const mavlink_command_long_t commandLong)
{
    emit requestOperatorControlReceived(commandLong.param1, commandLong.param3, commandLong.param4);
}

void Vehicle::_handleCommandLong(const mavlink_message_t& message)
{
    mavlink_command_long_t commandLong;
    mavlink_msg_command_long_decode(&message, &commandLong);
    // Ignore command if it is not targeted for us
    if (commandLong.target_system != MAVLinkProtocol::instance()->getSystemId()) {
        return;
    }
    if (commandLong.command == MAV_CMD_REQUEST_OPERATOR_CONTROL) {
        _handleCommandRequestOperatorControl(commandLong);
    }
}

int Vehicle::operatorControlTakeoverTimeoutMsecs() const
{
    return REQUEST_OPERATOR_CONTROL_ALLOW_TAKEOVER_TIMEOUT_MSECS;
}

int32_t Vehicle::getMessageRate(uint8_t compId, uint16_t msgId)
{
    return _messageIntervalManager->getMessageRate(compId, msgId);
}

void Vehicle::setMessageRate(uint8_t compId, uint16_t msgId, int32_t rate)
{
    _messageIntervalManager->setMessageRate(compId, msgId, rate);
}

QVariant Vehicle::expandedToolbarIndicatorSource(const QString& indicatorName)
{
    return _firmwarePlugin->expandedToolbarIndicatorSource(this, indicatorName);
}

QString Vehicle::requestMessageResultHandlerFailureCodeToString(RequestMessageResultHandlerFailureCode_t failureCode)
{
    return RequestMessageCoordinator::failureCodeToString(failureCode);
}

QString Vehicle::mavCmdResultFailureCodeToString(MavCmdResultFailureCode_t failureCode)
{
    return MavCommandQueue::failureCodeToString(failureCode);
}

/*===========================================================================*/
/*                         ardupilotmega Dialect                             */
/*===========================================================================*/

void Vehicle::flashBootloader()
{
    if (apmFirmware()) {
        sendMavCommand(
            defaultComponentId(),
            MAV_CMD_FLASH_BOOTLOADER,
            true,        // show error
            0, 0, 0, 0,  // param 1-4 not used
            290876);     // magic number
    }
}

void Vehicle::motorInterlock(bool enable)
{
    if (apmFirmware()) {
        sendMavCommand(
            defaultComponentId(),
            MAV_CMD_DO_AUX_FUNCTION,
            true,
            APM::AUX_FUNC::MOTOR_INTERLOCK,
            enable ? MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL_HIGH : MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL_LOW);
    }
}

/*---------------------------------------------------------------------------*/
/*===========================================================================*/
/*                         Status Text Handler                               */
/*===========================================================================*/

void Vehicle::resetAllMessages() { m_statusTextHandler->resetAllMessages(); }
void Vehicle::resetErrorLevelMessages() { m_statusTextHandler->resetErrorLevelMessages(); }
void Vehicle::clearMessages() { m_statusTextHandler->clearMessages(); }
bool Vehicle::messageTypeNone() const { return m_statusTextHandler->messageTypeNone(); }
bool Vehicle::messageTypeNormal() const { return m_statusTextHandler->messageTypeNormal(); }
bool Vehicle::messageTypeWarning() const { return m_statusTextHandler->messageTypeWarning(); }
bool Vehicle::messageTypeError() const { return m_statusTextHandler->messageTypeError(); }
int Vehicle::messageCount() const { return m_statusTextHandler->messageCount(); }
QString Vehicle::formattedMessages() const { return m_statusTextHandler->formattedMessages(); }

void Vehicle::_createStatusTextHandler()
{
    m_statusTextHandler = new StatusTextHandler(this);
    (void) connect(m_statusTextHandler, &StatusTextHandler::messageTypeChanged, this, &Vehicle::messageTypeChanged);
    (void) connect(m_statusTextHandler, &StatusTextHandler::messageCountChanged, this, &Vehicle::messageCountChanged);
    (void) connect(m_statusTextHandler, &StatusTextHandler::newFormattedMessage, this, &Vehicle::newFormattedMessage);
    (void) connect(m_statusTextHandler, &StatusTextHandler::textMessageReceived, this, &Vehicle::_textMessageReceived);
    (void) connect(m_statusTextHandler, &StatusTextHandler::newErrorMessage, this, &Vehicle::_errorMessageReceived);
}

void Vehicle::_onStatusTextFromEvent(uint8_t compid, int severity, const QString &text, const QString &description)
{
    m_statusTextHandler->handleHTMLEscapedTextMessage(static_cast<MAV_COMPONENT>(compid),
                                                      static_cast<MAV_SEVERITY>(severity), text, description);
}

void Vehicle::_textMessageReceived(MAV_COMPONENT componentid, MAV_SEVERITY severity, QString text, QString description)
{
    // PX4 backwards compatibility: messages sent out ending with a tab are also sent as event
    if (px4Firmware() && text.endsWith('\t')) {
        qCDebug(VehicleLog) << "Dropping message (expected as event):" << text;
        return;
    }

    bool skipSpoken = false;
    const bool ardupilotPrearm = text.startsWith(QStringLiteral("PreArm"));
    const bool px4Prearm = text.startsWith(QStringLiteral("preflight"), Qt::CaseInsensitive) && (severity >= MAV_SEVERITY::MAV_SEVERITY_CRITICAL);
    if (ardupilotPrearm || px4Prearm) {
        if (_healthAndArmingChecksSupported(componentid)) {
            qCDebug(VehicleLog) << "Dropping preflight message (expected as event):" << text;
            return;
        }

        // Limit repeated PreArm message to once every 10 seconds
        if (_noisySpokenPrearmMap.contains(text) && _noisySpokenPrearmMap.value(text).msecsTo(QTime::currentTime()) < (10 * 1000)) {
            skipSpoken = true;
        } else {
            (void) _noisySpokenPrearmMap.insert(text, QTime::currentTime());
            setPrearmError(text);
        }
    }

    bool readAloud = false;

    if (text.startsWith("#")) {
        (void) text.remove(0, 1);
        readAloud = true;
    } else if (severity <= MAV_SEVERITY::MAV_SEVERITY_NOTICE) {
        readAloud = true;
    }

    if (readAloud && !skipSpoken) {
        _say(text);
    }

    emit textMessageReceived(id(), componentid, severity, text, description);
    m_statusTextHandler->handleHTMLEscapedTextMessage(componentid, severity, text.toHtmlEscaped(), description);
}

void Vehicle::_errorMessageReceived(QString message)
{
    QString vehicleIdPrefix;

    if (MultiVehicleManager::instance()->vehicles()->count() > 1) {
        vehicleIdPrefix = tr("Vehicle %1: ").arg(id());
    }
    QGC::showCriticalVehicleMessage(vehicleIdPrefix + message);
}

/*---------------------------------------------------------------------------*/
/*===========================================================================*/
/*                                 Signing                                   */
/*===========================================================================*/

void Vehicle::sendSetupSigning(int keyIndex)
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "Primary Link Gone!";
        return;
    }

    const QByteArray keyBytes = MAVLinkSigningKeys::instance()->keyBytesAt(keyIndex);
    if (keyBytes.isEmpty()) {
        qCCritical(VehicleLog) << "Invalid key index:" << keyIndex;
        return;
    }

    const mavlink_channel_t channel = static_cast<mavlink_channel_t>(sharedLink->mavlinkChannel());

    mavlink_setup_signing_t setup_signing;

    mavlink_system_t target_system;
    target_system.sysid = id();
    target_system.compid = defaultComponentId();

    MAVLinkSigning::createSetupSigning(channel, target_system, keyBytes, setup_signing);

    // Also configure signing on our channel with this key so outgoing packets are signed
    if (!MAVLinkSigning::initSigning(channel, keyBytes, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback)) {
        qCCritical(VehicleLog) << "Internal error: failed to initialize signing on channel" << channel;
        return;
    }

    _mavlinkSigning = true;
    _mavlinkSigningKeyName = MAVLinkSigningKeys::instance()->keyNameAt(keyIndex);
    emit mavlinkSigningChanged();

    mavlink_message_t msg;
    (void) mavlink_msg_setup_signing_encode_chan(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::getComponentId(), channel, &msg, &setup_signing);

    // Since we don't get an ack back that the message was received send twice to try to make sure it makes it to the vehicle
    for (uint8_t i = 0; i < 2; ++i) {
        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void Vehicle::sendDisableSigning()
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "Primary Link Gone!";
        return;
    }

    const mavlink_channel_t channel = static_cast<mavlink_channel_t>(sharedLink->mavlinkChannel());

    mavlink_setup_signing_t setup_signing;

    mavlink_system_t target_system;
    target_system.sysid = id();
    target_system.compid = defaultComponentId();

    MAVLinkSigning::createDisableSigning(target_system, setup_signing);

    mavlink_message_t msg;
    (void) mavlink_msg_setup_signing_encode_chan(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::getComponentId(), channel, &msg, &setup_signing);

    for (uint8_t i = 0; i < 2; ++i) {
        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }

    // Disable signing on the local channel so we stop signing outgoing packets
    MAVLinkSigning::initSigning(channel, QByteArrayView(), nullptr);

    _mavlinkSigning = false;
    _mavlinkSigningKeyName.clear();
    emit mavlinkSigningChanged();
}

/*---------------------------------------------------------------------------*/
/*===========================================================================*/
/*                        Image Protocol Manager                             */
/*===========================================================================*/

void Vehicle::_createImageProtocolManager()
{
    _imageProtocolManager = new ImageProtocolManager(this);
    (void) connect(_imageProtocolManager, &ImageProtocolManager::flowImageIndexChanged, this, &Vehicle::flowImageIndexChanged);
    (void) connect(_imageProtocolManager, &ImageProtocolManager::imageReady, this, [this](const QImage &image) {
        qgcApp()->qgcImageProvider()->setImage(image, _systemID);
    });
}

uint32_t Vehicle::flowImageIndex() const
{
    return (_imageProtocolManager ? _imageProtocolManager->flowImageIndex() : 0);
}

/*---------------------------------------------------------------------------*/
/*===========================================================================*/
/*                         MAVLink Log Manager                               */
/*===========================================================================*/

void Vehicle::_createMAVLinkLogManager()
{
    _mavlinkLogManager = new MAVLinkLogManager(this, this);
}

MAVLinkLogManager *Vehicle::mavlinkLogManager() const
{
    return _mavlinkLogManager;
}

/*---------------------------------------------------------------------------*/
/*===========================================================================*/
/*                             Camera Manager                                */
/*===========================================================================*/

void Vehicle::_createCameraManager()
{
    if (!_cameraManager && _firmwarePlugin) {
        _cameraManager = _firmwarePlugin->createCameraManager(this);
        emit cameraManagerChanged();
    }
}

const QVariantList &Vehicle::staticCameraList() const
{
    if (_cameraManager) {
        return _cameraManager->cameraList();
    }

    static QVariantList emptyCameraList;
    return emptyCameraList;
}

/*---------------------------------------------------------------------------*/
/*===========================================================================*/
/*                          MAVLinkEventsManager                             */
/*===========================================================================*/

void Vehicle::_createMAVLinkEventManager()
{
    _eventManager = std::make_unique<MAVLinkEventManager>(this);

    (void) connect(_eventManager.get(), &MAVLinkEventManager::statusTextMessageFromEvent, this, &Vehicle::_onStatusTextFromEvent);
}

void Vehicle::_handleEventMessage(const mavlink_message_t& msg)
{
    _eventManager->handleEventMessage(msg);
}

bool Vehicle::_healthAndArmingChecksSupported(uint8_t compid)
{
    return _eventManager->healthAndArmingChecksSupported(compid);
}

HealthAndArmingCheckReport* Vehicle::healthAndArmingCheckReport()
{
    return _eventManager->healthAndArmingCheckReport();
}

void Vehicle::setEventsMetadata(uint8_t compid, const QString &metadataJsonFileName)
{
    _eventManager->setMetadata(compid, metadataJsonFileName);

    sendMavCommand(_defaultComponentId, MAV_CMD_RUN_PREARM_CHECKS, false);
}

/*---------------------------------------------------------------------------*/
