/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Vehicle.h"
#include "Actuators.h"
#include "ADSBVehicleManager.h"
#include "AudioOutput.h"
#include "AutoPilotPlugin.h"
#include "ComponentInformationManager.h"
#include "EventHandler.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "FTPManager.h"
#include "GeoFenceManager.h"
#include "ImageProtocolManager.h"
#include "InitialConnectStateMachine.h"
#include "Joystick.h"
#include "JoystickManager.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MissionCommandTree.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "PlanMasterController.h"
#include "PositionManager.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "QGCCorePlugin.h"
#include "QGCImageProvider.h"
#include "QGCQGeoCoordinate.h"
#include "RallyPointManager.h"
#include "RemoteIDManager.h"
#include "SettingsManager.h"
#include "StandardModes.h"
#include "TerrainProtocolHandler.h"
#include "TerrainQuery.h"
#include "TrajectoryPoints.h"
#include "VehicleBatteryFactGroup.h"
#include "VehicleLinkManager.h"
#include "VehicleObjectAvoidance.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include <DeviceInfo.h>
#include <StatusTextHandler.h>
#include <MAVLinkSigning.h>
#include "GimbalController.h"

#ifdef QGC_UTM_ADAPTER
#include "UTMSPVehicle.h"
#include "UTMSPManager.h"
#endif
#ifdef QT_DEBUG
#include "MockLink.h"
#endif

#include <QtCore/QDateTime>

QGC_LOGGING_CATEGORY(VehicleLog, "VehicleLog")

#define UPDATE_TIMER 50
#define DEFAULT_LAT  38.965767f
#define DEFAULT_LON -120.083923f
#define SET_HOME_TERRAIN_ALT_MAX 10000
#define SET_HOME_TERRAIN_ALT_MIN -500

const QString guided_mode_not_supported_by_vehicle = QObject::tr("Guided mode not supported by Vehicle.");

// Standard connected vehicle
Vehicle::Vehicle(LinkInterface*             link,
                 int                        vehicleId,
                 int                        defaultComponentId,
                 MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 QObject*                   parent)
    : VehicleFactGroup              (parent)
    , _id                           (vehicleId)
    , _defaultComponentId           (defaultComponentId)
    , _firmwareType                 (firmwareType)
    , _vehicleType                  (vehicleType)
    , _toolbox                      (qgcApp()->toolbox())
    , _settingsManager              (_toolbox->settingsManager())
    , _defaultCruiseSpeed           (_settingsManager->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed            (_settingsManager->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _firmwarePluginManager        (firmwarePluginManager)
    , _trajectoryPoints             (new TrajectoryPoints(this, this))
    , _mavlinkStreamConfig          (std::bind(&Vehicle::_setMessageInterval, this, std::placeholders::_1, std::placeholders::_2))
    , _vehicleFactGroup             (this)
    , _gpsFactGroup                 (this)
    , _gps2FactGroup                (this)
    , _windFactGroup                (this)
    , _vibrationFactGroup           (this)
    , _temperatureFactGroup         (this)
    , _clockFactGroup               (this)
    , _setpointFactGroup            (this)
    , _distanceSensorFactGroup      (this)
    , _localPositionFactGroup       (this)
    , _localPositionSetpointFactGroup(this)
    , _escStatusFactGroup           (this)
    , _estimatorStatusFactGroup     (this)
    , _hygrometerFactGroup          (this)
    , _generatorFactGroup           (this)
    , _efiFactGroup                 (this)
    , _terrainFactGroup             (this)
    , _terrainProtocolHandler       (new TerrainProtocolHandler(this, &_terrainFactGroup, this))
{
    _linkManager = _toolbox->linkManager();

    connect(JoystickManager::instance(), &JoystickManager::activeJoystickChanged, this, &Vehicle::_loadJoystickSettings);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &Vehicle::_activeVehicleChanged);

    _mavlink = _toolbox->mavlinkProtocol();
    qCDebug(VehicleLog) << "Link started with Mavlink " << (_mavlink->getCurrentVersion() >= 200 ? "V2" : "V1");

    connect(_mavlink, &MAVLinkProtocol::messageReceived,        this, &Vehicle::_mavlinkMessageReceived);
    connect(_mavlink, &MAVLinkProtocol::mavlinkMessageStatus,   this, &Vehicle::_mavlinkMessageStatus);

    connect(this, &Vehicle::flightModeChanged,          this, &Vehicle::_handleFlightModeChanged);
    connect(this, &Vehicle::armedChanged,               this, &Vehicle::_announceArmedChanged);
    connect(this, &Vehicle::flyingChanged, this, [this](bool flying){
        if (flying) {
            setInitialGCSPressure(QGCDeviceInfo::QGCPressure::instance()->pressure());
            setInitialGCSTemperature(QGCDeviceInfo::QGCPressure::instance()->temperature());
        }
    });

    connect(_toolbox->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &Vehicle::_vehicleParamLoaded);

    connect(this, &Vehicle::remoteControlRSSIChanged,   this, &Vehicle::_remoteControlRSSIChanged);

    _commonInit();

    _vehicleLinkManager->_addLink(link);

    // Set video stream to udp if running ArduSub and Video is disabled
    if (sub() && _settingsManager->videoSettings()->videoSource()->rawValue() == VideoSettings::videoDisabled) {
        _settingsManager->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
        _settingsManager->videoSettings()->lowLatencyMode()->setRawValue(true);
    }

#ifdef QGC_UTM_ADAPTER
    UTMSPManager* utmspManager = _toolbox->utmspManager();
    if (utmspManager) {
        _utmspVehicle = utmspManager->instantiateVehicle(*this);
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

    // Start timer to limit altitude above terrain queries
    _altitudeAboveTerrQueryTimer.restart();
}

// Disconnected Vehicle for offline editing
Vehicle::Vehicle(MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 QObject*                   parent)
    : VehicleFactGroup                  (parent)
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
    , _mavlinkStreamConfig              (std::bind(&Vehicle::_setMessageInterval, this, std::placeholders::_1, std::placeholders::_2))
    , _vehicleFactGroup                 (this)
    , _gpsFactGroup                     (this)
    , _gps2FactGroup                    (this)
    , _windFactGroup                    (this)
    , _vibrationFactGroup               (this)
    , _clockFactGroup                   (this)
    , _distanceSensorFactGroup          (this)
    , _localPositionFactGroup           (this)
    , _localPositionSetpointFactGroup   (this)
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
    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateAltAboveTerrain);
    // Initialize alt above terrain to Nan so frontend can display it correctly in case the terrain query had no response
    _altitudeAboveTerrFact.setRawValue(qQNaN());

    connect(_toolbox->qgcPositionManager(), &QGCPositionManager::gcsPositionChanged, this, &Vehicle::_updateDistanceToGCS);
    connect(_toolbox->qgcPositionManager(), &QGCPositionManager::gcsPositionChanged, this, &Vehicle::_updateHomepoint);

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
    _componentInformationManager    = new ComponentInformationManager   (this);
    _initialConnectStateMachine     = new InitialConnectStateMachine    (this);
    _ftpManager                     = new FTPManager                    (this);

    _vehicleLinkManager             = new VehicleLinkManager            (this);

    connect(_standardModes, &StandardModes::modesUpdated, this, &Vehicle::flightModesChanged);
    connect(_standardModes, &StandardModes::modesUpdated, this, [this](){ Vehicle::flightModeChanged(flightMode()); });

    _parameterManager = new ParameterManager(this);
    connect(_parameterManager, &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);

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
    connect(_toolbox->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &Vehicle::flightModesChanged);

    _createImageProtocolManager();
    _createStatusTextHandler();

    // _addFactGroup(_vehicleFactGroup,            _vehicleFactGroupName);
    _addFactGroup(&_gpsFactGroup,               _gpsFactGroupName);
    _addFactGroup(&_gps2FactGroup,              _gps2FactGroupName);
    _addFactGroup(&_windFactGroup,              _windFactGroupName);
    _addFactGroup(&_vibrationFactGroup,         _vibrationFactGroupName);
    _addFactGroup(&_temperatureFactGroup,       _temperatureFactGroupName);
    _addFactGroup(&_clockFactGroup,             _clockFactGroupName);
    _addFactGroup(&_setpointFactGroup,          _setpointFactGroupName);
    _addFactGroup(&_distanceSensorFactGroup,    _distanceSensorFactGroupName);
    _addFactGroup(&_localPositionFactGroup,     _localPositionFactGroupName);
    _addFactGroup(&_localPositionSetpointFactGroup,_localPositionSetpointFactGroupName);
    _addFactGroup(&_escStatusFactGroup,         _escStatusFactGroupName);
    _addFactGroup(&_estimatorStatusFactGroup,   _estimatorStatusFactGroupName);
    _addFactGroup(&_hygrometerFactGroup,        _hygrometerFactGroupName);
    _addFactGroup(&_generatorFactGroup,         _generatorFactGroupName);
    _addFactGroup(&_efiFactGroup,               _efiFactGroupName);
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

    // enable Joystick if appropriate
    _loadJoystickSettings();

    _gimbalController = new GimbalController(_mavlink, this);
}

Vehicle::~Vehicle()
{
    qCDebug(VehicleLog) << "~Vehicle" << this;

    delete _missionManager;
    _missionManager = nullptr;

    delete _autopilotPlugin;
    _autopilotPlugin = nullptr;

#ifdef QGC_UTM_ADAPTER
    delete _utmspVehicle;
#endif
}

void Vehicle::prepareDelete()
{
#if 0
    // I believe this should no longer be needed with new PhtoVideoControl implmenentation.
    // Leaving in for now, just in case it need to come back.
    if(_cameraManager) {
        // because of _cameraManager QML bindings check for nullptr won't work in the binding pipeline
        // the dangling pointer access will cause a runtime fault
        auto tmpCameras = _cameraManager;
        _cameraManager = nullptr;
        delete tmpCameras;
        emit cameraManagerChanged();
        qApp->processEvents();
    }
#endif
}

void Vehicle::deleteCameraManager()
{
    if(_cameraManager) {
        delete _cameraManager;
        _cameraManager = nullptr;
    }
}

void Vehicle::deleteGimbalController()
{
    if (_gimbalController) {
        delete _gimbalController;
        _gimbalController = nullptr;
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
    // If the link is already running at Mavlink V2 set our max proto version to it.
    unsigned mavlinkVersion = _mavlink->getCurrentVersion();
    if (_maxProtoVersion != mavlinkVersion && mavlinkVersion >= 200) {
        _maxProtoVersion = mavlinkVersion;
        qCDebug(VehicleLog) << "_mavlinkMessageReceived Link already running Mavlink v2. Setting _maxProtoVersion" << _maxProtoVersion;
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
    (void) QMetaObject::invokeMethod(_imageProtocolManager, "mavlinkMessageReceived", Qt::AutoConnection, message);
    _remoteIDManager->mavlinkMessageReceived(message);

    _waitForMavlinkMessageMessageReceivedHandler(message);

    // Battery fact groups are created dynamically as new batteries are discovered
    VehicleBatteryFactGroup::handleMessageForFactGroupCreation(this, message);

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
        _handleADSBVehicle(message);
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
        _eventHandler(message.compid).handleEvents(message);
        break;

    case MAVLINK_MSG_ID_SERIAL_CONTROL:
    {
        mavlink_serial_control_t ser;
        mavlink_msg_serial_control_decode(&message, &ser);
        if (static_cast<size_t>(ser.count) > sizeof(ser.data)) {
            qWarning() << "Invalid count for SERIAL_CONTROL, discarding." << ser.count;
        } else {
            emit mavlinkSerialControl(ser.device, ser.flags, ser.timeout, ser.baudrate,
                    QByteArray(reinterpret_cast<const char*>(ser.data), ser.count));
        }
    }
        break;
#ifdef DAILY_BUILD // Disable use of development/WIP MAVLink messages for release builds
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
#endif // DAILY_BUILD

        // Following are ArduPilot dialect messages
#if !defined(NO_ARDUPILOT_DIALECT)
    case MAVLINK_MSG_ID_CAMERA_FEEDBACK:
        _handleCameraFeedback(message);
        break;
#endif
    case MAVLINK_MSG_ID_LOG_ENTRY:
    {
        mavlink_log_entry_t log;
        mavlink_msg_log_entry_decode(&message, &log);
        emit logEntry(log.time_utc, log.size, log.id, log.num_logs, log.last_log_num);
        break;
    }
    case MAVLINK_MSG_ID_LOG_DATA:
    {
        mavlink_log_data_t log;
        mavlink_msg_log_data_decode(&message, &log);
        emit logData(log.ofs, log.id, log.count, log.data);
        break;
    }
    case MAVLINK_MSG_ID_MESSAGE_INTERVAL:
    {
        _handleMessageInterval(message);
        break;
    }
    }

    // This must be emitted after the vehicle processes the message. This way the vehicle state is up to date when anyone else
    // does processing.
    emit mavlinkMessageReceived(message);
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

// TODO: VehicleFactGroup
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

// TODO: VehicleFactGroup
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
        qCDebug(VehicleLog) << "new home location set at coordinate: " << homeCoord;
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
            if(_settingsManager->videoSettings()->disableWhenDisarmed()->rawValue().toBool()) {
                _settingsManager->videoSettings()->streamEnabled()->setRawValue(false);
                qgcApp()->toolbox()->videoManager()->stopVideo();
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

void Vehicle::_handleEvent(uint8_t comp_id, std::unique_ptr<events::parser::ParsedEvent> event)
{
    int severity = -1;
    switch (events::externalLogLevel(event->eventData().log_levels)) {
        case events::Log::Emergency: severity = MAV_SEVERITY_EMERGENCY; break;
        case events::Log::Alert: severity = MAV_SEVERITY_ALERT; break;
        case events::Log::Critical: severity = MAV_SEVERITY_CRITICAL; break;
        case events::Log::Error: severity = MAV_SEVERITY_ERROR; break;
        case events::Log::Warning: severity = MAV_SEVERITY_WARNING; break;
        case events::Log::Notice: severity = MAV_SEVERITY_NOTICE; break;
        case events::Log::Info: severity = MAV_SEVERITY_INFO; break;
        default: break;
    }

    // handle special groups & protocols
    if (event->group() == "health" || event->group() == "arming_check") {
        // these are displayed separately
        return;
    }
    if (event->group() == "calibration") {
        emit calibrationEventReceived(id(), comp_id, severity,
                QSharedPointer<events::parser::ParsedEvent>{new events::parser::ParsedEvent{*event}});
        // these are displayed separately
        return;
    }

    // show message according to the log level, don't show unknown event groups (might be part of a new protocol)
    if (event->group() == "default" && severity != -1) {
        std::string message = event->message();
        std::string description = event->description();

        if (event->type() == "append_health_and_arming_messages" && event->numArguments() > 0) {
            uint32_t customMode = event->argumentValue(0).value.val_uint32_t;
            const QSharedPointer<EventHandler>& eventHandler = _events[comp_id];
            int modeGroup = eventHandler->getModeGroup(customMode);
            std::vector<events::HealthAndArmingChecks::Check> checks = eventHandler->healthAndArmingCheckResults().checks(modeGroup);
            QList<std::string> messageChecks;
            for (const auto& check : checks) {
                if (events::externalLogLevel(check.log_levels) <= events::Log::Warning) {
                    messageChecks.append(check.message);
                }
            }
            if (messageChecks.empty()) {
                // Add all
                for (const auto& check : checks) {
                    messageChecks.append(check.message);
                }
            }
            if (!message.empty() && !messageChecks.empty()) {
                message += "\n";
            }
            if (messageChecks.size() == 1) {
                message += messageChecks[0];
            } else {
                for (const auto& messageCheck : messageChecks) {
                    message += "- " + messageCheck + "\n";
                }
            }
        }

        if (!message.empty()) {
            const QString text = QString::fromStdString(message);
            // Hack to prevent calibration messages from cluttering things up
            if (px4Firmware() && text.startsWith(QStringLiteral("[cal]"))) {
                return;
            }

            m_statusTextHandler->handleHTMLEscapedTextMessage(static_cast<MAV_COMPONENT>(comp_id), static_cast<MAV_SEVERITY>(severity), text, QString::fromStdString(description));
        }
    }
}

EventHandler& Vehicle::_eventHandler(uint8_t compid)
{
    auto eventData = _events.find(compid);
    if (eventData == _events.end()) {
        // add new component

        auto sendRequestEventMessageCB = [this](const mavlink_request_event_t& msg) {
            SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
            if (sharedLink) {
                mavlink_message_t message;
                mavlink_msg_request_event_encode_chan(_mavlink->getSystemId(),
                        _mavlink->getComponentId(),
                        sharedLink->mavlinkChannel(),
                        &message,
                        &msg);
                sendMessageOnLinkThreadSafe(sharedLink.get(), message);
            }
        };

        QString profile = "dev"; // TODO: should be configurable

        QSharedPointer<EventHandler> eventHandler{new EventHandler(this, profile,
                std::bind(&Vehicle::_handleEvent, this, compid, std::placeholders::_1),
                sendRequestEventMessageCB,
                _mavlink->getSystemId(), _mavlink->getComponentId(), _id, compid)};
        eventData = _events.insert(compid, eventHandler);

        // connect health and arming check updates
        connect(eventHandler.data(), &EventHandler::healthAndArmingChecksUpdated, this, [compid, this]() {
            const QSharedPointer<EventHandler>& eventHandler = _events[compid];
            _healthAndArmingCheckReport.update(compid, eventHandler->healthAndArmingCheckResults(),
                    eventHandler->getModeGroup(_has_custom_mode_user_intention ? _custom_mode_user_intention : _custom_mode));
        });
        connect(this, &Vehicle::flightModeChanged, this, [compid, this]() {
            const QSharedPointer<EventHandler>& eventHandler = _events[compid];
            if (eventHandler->healthAndArmingCheckResultsValid()) {
                _healthAndArmingCheckReport.update(compid, eventHandler->healthAndArmingCheckResults(),
                                                   eventHandler->getModeGroup(_has_custom_mode_user_intention ? _custom_mode_user_intention : _custom_mode));
            }
        });
    }
    return *eventData->data();
}

void Vehicle::setEventsMetadata(uint8_t compid, const QString& metadataJsonFileName)
{
    _eventHandler(compid).setMetadata(metadataJsonFileName);

    // get the mode group for some well-known flight modes
    int modeGroups[2]{-1, -1};
    const QString modes[2]{"Takeoff", "Mission"};
    for (size_t i = 0; i < sizeof(modeGroups)/sizeof(modeGroups[0]); ++i) {
        uint8_t     base_mode;
        uint32_t    custom_mode;
        if (setFlightModeCustom(modes[i], &base_mode, &custom_mode)) {
            modeGroups[i] = _eventHandler(compid).getModeGroup(custom_mode);
            if (modeGroups[i] == -1) {
                qCDebug(VehicleLog) << "Failed to get mode group for mode" << modes[i] << "(Might not be in metadata)";
            }
        }
    }
    _healthAndArmingCheckReport.setModeGroups(modeGroups[0], modeGroups[1]);

    // Request arming checks to be reported
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_RUN_PREARM_CHECKS,
                   false);
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
    mavlink_current_mode_t currentMode;
    mavlink_msg_current_mode_decode(&message, &currentMode);
    if (currentMode.intended_custom_mode != 0) { // 0 == unknown/not supplied
        _has_custom_mode_user_intention = true;
        bool changed = _custom_mode_user_intention != currentMode.intended_custom_mode;
        _custom_mode_user_intention = currentMode.intended_custom_mode;
        if (changed) {
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

    uint16_t* _rgChannelvalues[QGCMAVLink::maxRcChannels] = {
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
    int pwmValues[QGCMAVLink::maxRcChannels];

    for (int i=0; i<QGCMAVLink::maxRcChannels; i++) {
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

// this function called in three cases:
// 1. On constructor of vehicle, to see if we should enable a joystick
// 2. When there is a new active joystick
// 3. When the active joystick is disconnected (even if there isnt a new one)
void Vehicle::_loadJoystickSettings()
{
    QSettings settings;
    settings.beginGroup(QString(_settingsGroup).arg(_id));

    if (JoystickManager::instance()->activeJoystick()) {
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Notified of an active joystick. Loading setting joystickenabled: " << settings.value(_joystickEnabledSettingsKey, false).toBool();
        setJoystickEnabled(settings.value(_joystickEnabledSettingsKey, false).toBool());
    } else {
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Notified that there is no active joystick";
        setJoystickEnabled(false);
    }
}

// This is called from the UI when a deliberate action is taken to enable or disable the joystick
// This save allows the joystick enable state to persist restarts, disconnections of the joystick etc
void Vehicle::saveJoystickSettings()
{
    QSettings settings;
    settings.beginGroup(QString(_settingsGroup).arg(_id));

    // The joystick enabled setting should only be changed if a joystick is present
    // since the checkbox can only be clicked if one is present
    if (JoystickManager::instance()->joysticks().count()) {
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Saving setting joystickenabled: " << _joystickEnabled;
        settings.setValue(_joystickEnabledSettingsKey, _joystickEnabled);
    }
}

bool Vehicle::joystickEnabled() const
{
    return _joystickEnabled;
}

void Vehicle::setJoystickEnabled(bool enabled)
{
    if (enabled){
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Joystick Enabled";
    }
    else {
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Joystick Disabled";
    }

    // _joystickEnabled is the runtime state - it determines whether a vehicle is using joystick data when it is active
    _joystickEnabled = enabled;

    // if we are the active vehicle, call start polling on the active joystick
    // This routes the joystick signals to this vehicle
    if (enabled && _toolbox->multiVehicleManager()->activeVehicle() == this){
        _captureJoystick();
    }

    emit joystickEnabledChanged(_joystickEnabled);
}

void Vehicle::_activeVehicleChanged(Vehicle *newActiveVehicle)
{
    // the new active vehicle should always capture the joystick
    // even if the new active vehicle has joystick disabled
    // capturing the joystick will stop the joystick data going to the inactive vehicle
    if (newActiveVehicle == this){
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " is the new active vehicle";
        _captureJoystick();
        _isActiveVehicle = true;
    } else {
        _isActiveVehicle = false;
    }
}

// tells the active joystick where to send data
void Vehicle::_captureJoystick()
{
    Joystick* joystick = JoystickManager::instance()->activeJoystick();

    if(joystick){
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Capture Joystick" << joystick->name();
        joystick->startPolling(this);
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
	if (_standardModes->supported()) {
		return _standardModes->flightModes();
	}
    return _firmwarePlugin->flightModes(this);
}

QString Vehicle::flightMode() const
{
    if (_standardModes->supported()) {
        return _standardModes->flightMode(_custom_mode);
    }
    return _firmwarePlugin->flightMode(_base_mode, _custom_mode);
}

bool Vehicle::setFlightModeCustom(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    if (_standardModes->supported()) {
        *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
        return _standardModes->setFlightMode(flightMode, custom_mode);
    }
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

void Vehicle::_sendMessageMultipleNext()
{
    if (_nextSendMessageMultipleIndex < _sendMessageMultipleList.count()) {
        qCDebug(VehicleLog) << "_sendMessageMultipleNext:" << _sendMessageMultipleList[_nextSendMessageMultipleIndex].message.msgid;

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
    qCDebug(VehicleLog) << "_parametersReady" << parametersReady;

    // Try to set current unix time to the vehicle
    _sendQGCTimeToVehicle();
    // Send time twice, more likely to get to the vehicle on a noisy link
    _sendQGCTimeToVehicle();
    if (parametersReady) {
        disconnect(_parameterManager, &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);
        _setupAutoDisarmSignalling();
        _initialConnectStateMachine->advance();
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
    mavlink_msg_system_time_encode_chan(_mavlink->getSystemId(),
                                        _mavlink->getComponentId(),
                                        sharedLink->mavlinkChannel(),
                                        &msg,
                                        &cmd);

    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
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

bool Vehicle::changeHeadingSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::ChangeHeadingCapability);
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

double Vehicle::minimumTakeoffAltitudeMeters()
{
    return _firmwarePlugin->minimumTakeoffAltitudeMeters(this);
}

double Vehicle::maximumHorizontalSpeedMultirotor()
{
    return _firmwarePlugin->maximumHorizontalSpeedMultirotor(this);
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

void Vehicle::guidedModeChangeAltitude(double altitudeChange, bool pauseVehicle)
{
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeAltitude(this, altitudeChange, pauseVehicle);
}

void
Vehicle::guidedModeChangeGroundSpeedMetersSecond(double groundspeed)
{
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeGroundSpeedMetersSecond(this, groundspeed);
}

void
Vehicle::guidedModeChangeEquivalentAirspeedMetersSecond(double airspeed)
{
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeEquivalentAirspeedMetersSecond(this, airspeed);
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
    // Sanity check Ardupilot. Max altitude processed is 83000
    if (apmFirmware()) {
        if ((centerCoord.altitude() >= 83000) || (centerCoord.altitude() <= -83000  )) {
            return;
        }
    }
    if (!roiModeSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("ROI mode not supported by Vehicle."));
        return;
    }
    if (capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        sendMavCommandInt(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_LOCATION,
                    apmFirmware() ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
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
    // This is picked by qml to display coordinate over map
    emit roiCoordChanged(centerCoord);
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

void Vehicle::guidedModeChangeHeading(const QGeoCoordinate &headingCoord)
{
    if (!changeHeadingSupported()) {
        qgcApp()->showAppMessage(tr("Change Heading not supported by Vehicle."));
        return;
    }

    _firmwarePlugin->guidedModeChangeHeading(this, headingCoord);
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
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "setCurrentMissionSequence: primary link gone!";
        return;
    }

    mavlink_message_t       msg;

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

void Vehicle::sendMavCommand(int compId, MAV_CMD command, bool showError, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _sendMavCommandWorker(false,            // commandInt
                          showError,
                          nullptr,          // no handlers
                          compId,
                          command,
                          MAV_FRAME_GLOBAL,
                          param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandDelayed(int compId, MAV_CMD command, bool showError, int milliseconds, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    QTimer::singleShot(milliseconds, this, [=] { sendMavCommand(compId, command, showError, param1, param2, param3, param4, param5, param6, param7); });
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
    _sendMavCommandWorker(false,                // commandInt
                          false,                // showError
                          ackHandlerInfo,
                          compId,
                          command,
                          MAV_FRAME_GLOBAL,
                          param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandInt(int compId, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    _sendMavCommandWorker(true,         // commandInt
                          showError,
                          nullptr,      // no handlers
                          compId,
                          command,
                          frame,
                          param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandIntWithHandler(const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, MAV_FRAME frame, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    _sendMavCommandWorker(true,                   // commandInt
                          false,                  // showError
                          ackHandlerInfo,
                          compId,
                          command,
                          frame,
                          param1, param2, param3, param4, param5, param6, param7);
}

bool Vehicle::isMavCommandPending(int targetCompId, MAV_CMD command)
{
    bool pending = ((-1) < _findMavCommandListEntryIndex(targetCompId, command));
    // qDebug() << "Pending target: " << targetCompId << ", command: " << (int)command << ", pending: " << (pending ? "yes" : "no");
    return pending;
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
    case MAV_CMD_RUN_PREARM_CHECKS:
        return true;

    default:
        return false;
    }
}

bool Vehicle::_commandCanBeDuplicated(MAV_CMD command)
{
    // For some commands we don't care about response as much as we care about sending them regularly.
    // This test avoids commands not being sent due to an ACK not being received yet.
    // MOTOR_TEST in ardusub is a case where we need a constant stream of commands so it doesn't time out.
    switch (command) {
    case MAV_CMD_DO_MOTOR_TEST:
        return true;
    default:
        return false;
    }
}

void Vehicle::_sendMavCommandWorker(
    bool        commandInt, 
    bool        showError, 
    const MavCmdAckHandlerInfo_t* ackHandlerInfo,
    int         targetCompId, 
    MAV_CMD     command, 
    MAV_FRAME   frame, 
    float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    // We can't send commands to compIdAll using this method. The reason being that we would get responses back possibly from multiple components
    // which this code can't handle.
    // We also can't send the majority of commands again if we are already waiting for a response from that same command. If we did that we would not be able to discern
    // which ack was associated with which command.
    if ((targetCompId == MAV_COMP_ID_ALL) || (isMavCommandPending(targetCompId, command) && !_commandCanBeDuplicated(command))) {
        bool    compIdAll       = targetCompId == MAV_COMP_ID_ALL;
        QString rawCommandName  = _toolbox->missionCommandTree()->rawName(command);

        qCDebug(VehicleLog) << QStringLiteral("_sendMavCommandWorker failing %1").arg(compIdAll ? "MAV_COMP_ID_ALL not supported" : "duplicate command") << rawCommandName << param1 << param2 << param3 << param4 << param5 << param6 << param7;

        MavCmdResultFailureCode_t failureCode = compIdAll ? MavCmdResultCommandResultOnly : MavCmdResultFailureDuplicateCommand;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (showError) {
            qgcApp()->showAppMessage(tr("Unable to send command: %1.").arg(compIdAll ? tr("Internal error - MAV_COMP_ID_ALL not supported") : tr("Waiting on previous response to same command.")));
        }

        return;
    }

    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "_sendMavCommandWorker: primary link gone!";
        return;
    }

    MavCommandListEntry_t   entry;

    entry.useCommandInt     = commandInt;
    entry.targetCompId      = targetCompId;
    entry.command           = command;
    entry.frame             = frame;
    entry.showError         = showError;
    entry.ackHandlerInfo    = {};
    if (ackHandlerInfo) {
        entry.ackHandlerInfo = *ackHandlerInfo;
    }
    entry.rgParam1          = param1;
    entry.rgParam2          = param2;
    entry.rgParam3          = param3;
    entry.rgParam4          = param4;
    entry.rgParam5          = param5;
    entry.rgParam6          = param6;
    entry.rgParam7          = param7;
    entry.maxTries          = _sendMavCommandShouldRetry(command) ? _mavCommandMaxRetryCount : 1;
    entry.ackTimeoutMSecs   = sharedLink->linkConfiguration()->isHighLatency() ? _mavCommandAckTimeoutMSecsHighLatency : _mavCommandAckTimeoutMSecs;
    entry.elapsedTimer.start();

    qCDebug(VehicleLog) << Q_FUNC_INFO << "command:param1-7" << command << param1 << param2 << param3 << param4 << param5 << param6 << param7;

    _mavCommandList.append(entry);
    _sendMavCommandFromList(_mavCommandList.count() - 1);
}

void Vehicle::_sendMavCommandFromList(int index)
{
    MavCommandListEntry_t commandEntry = _mavCommandList[index];

    QString rawCommandName  = _toolbox->missionCommandTree()->rawName(commandEntry.command);

    if (++_mavCommandList[index].tryCount > commandEntry.maxTries) {
        qCDebug(VehicleLog) << Q_FUNC_INFO << "giving up after max retries" << rawCommandName;
        _mavCommandList.removeAt(index);
        if (commandEntry.ackHandlerInfo.resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*commandEntry.ackHandlerInfo.resultHandler)(commandEntry.ackHandlerInfo.resultHandlerData, commandEntry.targetCompId, ack, MavCmdResultFailureNoResponseToCommand);
        } else {
            emit mavCommandResult(_id, commandEntry.targetCompId, commandEntry.command, MAV_RESULT_FAILED, MavCmdResultFailureNoResponseToCommand);
        }
        if (commandEntry.showError) {
            qgcApp()->showAppMessage(tr("Vehicle did not respond to command: %1").arg(rawCommandName));
        }
        return;
    }

    if (commandEntry.tryCount > 1 && !px4Firmware() && commandEntry.command == MAV_CMD_START_RX_PAIR) {
        // The implementation of this command comes from the IO layer and is shared across stacks. So for other firmwares
        // we aren't really sure whether they are correct or not.
        return;
    }

    qCDebug(VehicleLog) << Q_FUNC_INFO << "command:tryCount:param1-7" << rawCommandName << commandEntry.tryCount << commandEntry.rgParam1 << commandEntry.rgParam2 << commandEntry.rgParam3 << commandEntry.rgParam4 << commandEntry.rgParam5 << commandEntry.rgParam6 << commandEntry.rgParam7;

    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "_sendMavCommandFromList: primary link gone!";
        return;
    }

    mavlink_message_t  msg;

    if (commandEntry.useCommandInt) {
        mavlink_command_int_t  cmd;

        memset(&cmd, 0, sizeof(cmd));
        cmd.target_system =     _id;
        cmd.target_component =  commandEntry.targetCompId;
        cmd.command =           commandEntry.command;
        cmd.frame =             commandEntry.frame;
        cmd.param1 =            commandEntry.rgParam1;
        cmd.param2 =            commandEntry.rgParam2;
        cmd.param3 =            commandEntry.rgParam3;
        cmd.param4 =            commandEntry.rgParam4;
        cmd.x =                 commandEntry.frame == MAV_FRAME_MISSION ? commandEntry.rgParam5 : commandEntry.rgParam5 * 1e7;
        cmd.y =                 commandEntry.frame == MAV_FRAME_MISSION ? commandEntry.rgParam6 : commandEntry.rgParam6 * 1e7;
        cmd.z =                 commandEntry.rgParam7;
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
        cmd.param1 =            commandEntry.rgParam1;
        cmd.param2 =            commandEntry.rgParam2;
        cmd.param3 =            commandEntry.rgParam3;
        cmd.param4 =            commandEntry.rgParam4;
        cmd.param5 =            static_cast<float>(commandEntry.rgParam5);
        cmd.param6 =            static_cast<float>(commandEntry.rgParam6);
        cmd.param7 =            commandEntry.rgParam7;
        mavlink_msg_command_long_encode_chan(_mavlink->getSystemId(),
                                             _mavlink->getComponentId(),
                                             sharedLink->mavlinkChannel(),
                                             &msg,
                                             &cmd);
    }

    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
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
            _sendMavCommandFromList(i);
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

    if (ack.command == MAV_CMD_PREFLIGHT_STORAGE) {
        auto result = (ack.result == MAV_RESULT_ACCEPTED);
        emit sensorsParametersResetAck(result);
    }

#if !defined(NO_ARDUPILOT_DIALECT)
    if (ack.command == MAV_CMD_FLASH_BOOTLOADER && ack.result == MAV_RESULT_ACCEPTED) {
        qgcApp()->showAppMessage(tr("Bootloader flash succeeded"));
    }
#endif

    int entryIndex = _findMavCommandListEntryIndex(message.compid, static_cast<MAV_CMD>(ack.command));
    if (entryIndex != -1) {
        if (ack.result == MAV_RESULT_IN_PROGRESS) {
            MavCommandListEntry_t commandEntry;
            if (px4Firmware() && ack.command == MAV_CMD_DO_AUTOTUNE_ENABLE) {
                // HacK to support PX4 autotune which does not send final result ack and just sends in progress
                commandEntry = _mavCommandList.takeAt(entryIndex);
            } else {
                // Command has not completed yet, don't remove
                MavCommandListEntry_t& commandEntryRef = _mavCommandList[entryIndex];
                commandEntryRef.maxTries = 1;         // Vehicle responsed to command so don't retry
                commandEntryRef.elapsedTimer.start(); // We've heard from vehicle, restart elapsed timer for no ack received timeout
                commandEntry = commandEntryRef;
            }

            if (commandEntry.ackHandlerInfo.progressHandler) {
                (*commandEntry.ackHandlerInfo.progressHandler)(commandEntry.ackHandlerInfo.progressHandlerData, message.compid, ack);
            }
        } else {
            MavCommandListEntry_t commandEntry = _mavCommandList.takeAt(entryIndex);

            if (commandEntry.ackHandlerInfo.resultHandler) {
                (*commandEntry.ackHandlerInfo.resultHandler)(commandEntry.ackHandlerInfo.resultHandlerData, message.compid, ack, MavCmdResultCommandResultOnly);
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
    } else {
        qCDebug(VehicleLog) << "_handleCommandAck Ack not in list" << rawCommandName;
    }

    // advance PID tuning setup/teardown
    if (ack.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        _mavlinkStreamConfig.gotSetMessageIntervalAck();
    }
}

void Vehicle::_removeRequestMessageInfo(int compId, int msgId)
{
    if (_requestMessageInfoMap.contains(compId) && _requestMessageInfoMap[compId].contains(msgId)) {
        delete _requestMessageInfoMap[compId][msgId];
        _requestMessageInfoMap[compId].remove(msgId);
    } else {
        qWarning() << Q_FUNC_INFO << "compId:msgId not found" << compId << msgId;
    }
}


void Vehicle::_waitForMavlinkMessageMessageReceivedHandler(const mavlink_message_t& message)
{
    if (_requestMessageInfoMap.contains(message.compid) && _requestMessageInfoMap[message.compid].contains(message.msgid)) {
        auto pInfo              = _requestMessageInfoMap[message.compid][message.msgid];
        auto resultHandler      = pInfo->resultHandler;
        auto resultHandlerData  = pInfo->resultHandlerData;

        qCDebug(VehicleLog) << Q_FUNC_INFO << "message received - compId:msgId" << message.compid << message.msgid;

        if (!pInfo->commandAckReceived) {
            qCDebug(VehicleLog) << Q_FUNC_INFO << "message received before ack came back.";
            int entryIndex = _findMavCommandListEntryIndex(message.compid, MAV_CMD_REQUEST_MESSAGE);
            if (entryIndex != -1) {
                _mavCommandList.takeAt(entryIndex);
            } else {
                qWarning() << Q_FUNC_INFO << "Removing request message command from list failed - not found in list";
            }
        }
        _removeRequestMessageInfo(message.compid, message.msgid);

        (*resultHandler)(resultHandlerData, MAV_RESULT_ACCEPTED, RequestMessageNoFailure, message);
    } else {
        // We use any incoming message as a trigger to check timeouts on message requests

        for (auto& compIdEntry : _requestMessageInfoMap) {
            for (auto requestMessageInfo : compIdEntry) {    
                if (requestMessageInfo->messageWaitElapsedTimer.isValid() && requestMessageInfo->messageWaitElapsedTimer.elapsed() > (qgcApp()->runningUnitTests() ? 50 : 1000)) {
                    auto resultHandler      = requestMessageInfo->resultHandler;
                    auto resultHandlerData  = requestMessageInfo->resultHandlerData;

                    qCDebug(VehicleLog) << Q_FUNC_INFO << "request message timed out - compId:msgId" << requestMessageInfo->compId << requestMessageInfo->msgId;

                    _removeRequestMessageInfo(requestMessageInfo->compId, requestMessageInfo->msgId);

                    mavlink_message_t message;
                    (*resultHandler)(resultHandlerData, MAV_RESULT_FAILED, RequestMessageFailureMessageNotReceived, message);

                    return; // We only handle one timeout at a time
                }
            }
        }
    }
}

void Vehicle::_requestMessageCmdResultHandler(void* resultHandlerData_, [[maybe_unused]] int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    auto requestMessageInfo = static_cast<RequestMessageInfo_t*>(resultHandlerData_);
    auto resultHandler      = requestMessageInfo->resultHandler;
    auto resultHandlerData  = requestMessageInfo->resultHandlerData;
    auto vehicle            = requestMessageInfo->vehicle;
    auto message            = requestMessageInfo->message;

    requestMessageInfo->commandAckReceived = true;
    if (ack.result != MAV_RESULT_ACCEPTED) {
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

        vehicle->_removeRequestMessageInfo(requestMessageInfo->compId, requestMessageInfo->msgId);

        (*resultHandler)(resultHandlerData, static_cast<MAV_RESULT>(ack.result),  requestMessageFailureCode, message);

        return;
    }

    if (requestMessageInfo->messageReceived) {
        // This should never happen. The command should have already been removed from the list when the message was received
        qWarning() << Q_FUNC_INFO << "Command result handler should now have been called if message has already been received";
    } else {
        // Now that the request has been acked we start the timer to wait for the message
        requestMessageInfo->messageWaitElapsedTimer.start();
    }
}

void Vehicle::_requestMessageWaitForMessageResultHandler(void* resultHandlerData, bool noResponsefromVehicle, const mavlink_message_t& message)
{
    RequestMessageInfo_t* pInfo = static_cast<RequestMessageInfo_t*>(resultHandlerData);

    pInfo->messageReceived  = true;
    if (pInfo->commandAckReceived) {
        (*pInfo->resultHandler)(pInfo->resultHandlerData, noResponsefromVehicle ? MAV_RESULT_FAILED : MAV_RESULT_ACCEPTED, noResponsefromVehicle ? RequestMessageFailureMessageNotReceived : RequestMessageNoFailure, message);
    } else {
        // Result handler will be called when we get the Ack
        pInfo->message = message;
    }
}

void Vehicle::requestMessage(RequestMessageResultHandler resultHandler, void* resultHandlerData, int compId, int messageId, float param1, float param2, float param3, float param4, float param5)
{
    auto requestMessageInfo = new RequestMessageInfo_t;
    requestMessageInfo->vehicle                 = this;
    requestMessageInfo->compId                  = compId;
    requestMessageInfo->msgId                   = messageId;
    requestMessageInfo->resultHandler           = resultHandler;
    requestMessageInfo->resultHandlerData       = resultHandlerData;

    _requestMessageInfoMap[compId][messageId] = requestMessageInfo;

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo;
    handlerInfo.resultHandler       = _requestMessageCmdResultHandler;
    handlerInfo.resultHandlerData   = requestMessageInfo;

    _sendMavCommandWorker(false,                                    // commandInt
                          false,                                    // showError
                          &handlerInfo,
                          compId,
                          MAV_CMD_REQUEST_MESSAGE,
                          MAV_FRAME_GLOBAL,
                          messageId,
                          param1, param2, param3, param4, param5, 0);
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
        qgcApp()->showAppMessage(tr("Vehicle reboot failed."));
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
                _mavlink->getSystemId(),
                _mavlink->getComponentId(),
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

QVariant Vehicle::mainStatusIndicatorContentItem()
{
    if(_firmwarePlugin) {
        return _firmwarePlugin->mainStatusIndicatorContentItem(this);
    }
    return QVariant();
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

void Vehicle::_handleADSBVehicle(const mavlink_message_t &message)
{
    mavlink_adsb_vehicle_t adsbVehicleMsg;
    mavlink_msg_adsb_vehicle_decode(&message, &adsbVehicleMsg);

    static constexpr int maxTimeSinceLastSeen = 15;

    if ((adsbVehicleMsg.flags & ADSB_FLAGS_VALID_COORDS) && (adsbVehicleMsg.tslc <= maxTimeSinceLastSeen)) {
        ADSB::VehicleInfo_t vehicleInfo;

        vehicleInfo.availableFlags = ADSB::AvailableInfoTypes::fromInt(0);
        vehicleInfo.icaoAddress = adsbVehicleMsg.ICAO_address;

        vehicleInfo.location.setLatitude(adsbVehicleMsg.lat / 1e7);
        vehicleInfo.location.setLongitude(adsbVehicleMsg.lon / 1e7);
        vehicleInfo.availableFlags |= ADSB::LocationAvailable;

        vehicleInfo.callsign = adsbVehicleMsg.callsign;
        vehicleInfo.availableFlags |= ADSB::CallsignAvailable;

        if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_ALTITUDE) {
            vehicleInfo.altitude = adsbVehicleMsg.altitude / 1e3;
            vehicleInfo.availableFlags |= ADSB::AltitudeAvailable;
        }

        if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_HEADING) {
            vehicleInfo.heading = adsbVehicleMsg.heading / 1e2;
            vehicleInfo.availableFlags |= ADSB::HeadingAvailable;
        }

        (void) QMetaObject::invokeMethod(ADSBVehicleManager::instance(), "adsbVehicleUpdate", Qt::AutoConnection, vehicleInfo);
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

void Vehicle::_updateHomepoint()
{
    const bool setHomeCmdSupported = firmwarePlugin()->supportedMissionCommands(vehicleClass()).contains(MAV_CMD_DO_SET_HOME);
    const bool updateHomeActivated = _toolbox->settingsManager()->flyViewSettings()->updateHomePosition()->rawValue().toBool();
    if(setHomeCmdSupported && updateHomeActivated){
        QGeoCoordinate gcsPosition = _toolbox->qgcPositionManager()->gcsPosition();
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
    if(uasId == _id) {
        _mavlinkSentCount       = totalSent;
        _mavlinkReceivedCount   = totalReceived;
        _mavlinkLossCount       = totalLoss;
        _mavlinkLossPercent     = lossPercent;
        emit mavlinkStatusChanged();
    }
}

int Vehicle::versionCompare(QString& compare) const
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
    _setpointFactGroup.setLiveUpdates(liveUpdate);
    _localPositionFactGroup.setLiveUpdates(liveUpdate);
    _localPositionSetpointFactGroup.setLiveUpdates(liveUpdate);

    switch (mode) {
    case ModeDisabled:
        _mavlinkStreamConfig.restoreDefaults();
        break;
    case ModeRateAndAttitude:
        _mavlinkStreamConfig.setHighRateRateAndAttitude();
        break;
    case ModeVelocityAndPosition:
        _mavlinkStreamConfig.setHighRateVelAndPos();
        break;
    case ModeAltitudeAndAirspeed:
        _mavlinkStreamConfig.setHighRateAltAirspeed();
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

bool Vehicle::isInitialConnectComplete() const
{
    return !_initialConnectStateMachine->active();
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

void Vehicle::doSetHome(const QGeoCoordinate& coord)
{
    if (coord.isValid()) {
        // If for some reason we already did a query and it hasn't arrived yet, disconnect signals and unset current query. TerrainQuery system will
        // automatically delete that forgotten query. This could happen if we send 2 do_set_home commands one after another, so usually the latest one
        // is the one we would want to arrive to the vehicle, so it is fine to forget about the previous ones in case it could happen.
        if (_currentDoSetHomeTerrainAtCoordinateQuery) {
            disconnect(_currentDoSetHomeTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &Vehicle::_doSetHomeTerrainReceived);
            _currentDoSetHomeTerrainAtCoordinateQuery = nullptr;
        }
        // Save the coord for using when our terrain data arrives. If there was a pending terrain query paired with an older coordinate it is safe to 
        // Override now, as we just disconnected the signal that would trigger the command sending 
        _doSetHomeCoordinate = coord;
        // Now setup and trigger the new terrain query
        _currentDoSetHomeTerrainAtCoordinateQuery = new TerrainAtCoordinateQuery(true /* autoDelet */);
        connect(_currentDoSetHomeTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &Vehicle::_doSetHomeTerrainReceived);
        QList<QGeoCoordinate> rgCoord;
        rgCoord.append(coord);
        _currentDoSetHomeTerrainAtCoordinateQuery->requestData(rgCoord);
    }
}

// This will be called after our query started in doSetHome arrives
void Vehicle::_doSetHomeTerrainReceived(bool success, QList<double> heights)
{
    if (success) {
        double terrainAltitude = heights[0];
        // Coord and terrain alt sanity check
        if (_doSetHomeCoordinate.isValid() && terrainAltitude <= SET_HOME_TERRAIN_ALT_MAX && terrainAltitude >= SET_HOME_TERRAIN_ALT_MIN) {
            sendMavCommand(
                        defaultComponentId(),
                        MAV_CMD_DO_SET_HOME,
                        true, // show error if fails
                        0,
                        0,
                        0,
                        static_cast<float>(qQNaN()),
                        _doSetHomeCoordinate.latitude(),
                        _doSetHomeCoordinate.longitude(),
                        terrainAltitude);

        } else if (_doSetHomeCoordinate.isValid()) {
            qCDebug(VehicleLog) << "_doSetHomeTerrainReceived: internal error, elevation data out of limits ";
        } else {
            qCDebug(VehicleLog) << "_doSetHomeTerrainReceived: internal error, cached home coordinate is not valid";
        }
    } else {
        qgcApp()->showAppMessage(tr("Set Home failed, terrain data not available for selected coordinate"));
    }
    // Clean up
    _currentDoSetHomeTerrainAtCoordinateQuery = nullptr;
    _doSetHomeCoordinate = QGeoCoordinate(); // So isValid() will no longer return true, for extra safety
}

void Vehicle::_updateAltAboveTerrain()
{
    // We won't do another query if the previous query was done closer than 2 meters from current position
    // or if altitude change has been less than 0.5 meters since then.
    const qreal minimumDistanceTraveled = 2;
    const float minimumAltitudeChanged  = 0.5f;

    // This is not super elegant but it works to limit the amount of queries we do. It seems more than 500ms is not possible to get
    // serviced on time. It is not a big deal if it is not serviced on time as terrain queries can manage that just fine, but QGC would
    // use resources to service those queries, and it is pointless, so this is a quick workaround to not waste that little computing time
    int altitudeAboveTerrQueryMinInterval = 500;
    if (_altitudeAboveTerrQueryTimer.elapsed() < altitudeAboveTerrQueryMinInterval) {
        // qCDebug(VehicleLog) << "_updateAltAboveTerrain: minimum 500ms interval between queries not reached, returning";
        return;
    }
    // Sanity check, although it is very unlikely that vehicle coordinate is not valid
    if (_coordinate.isValid()) {
        // Check for minimum distance and altitude traveled before doing query, to not do a lot of queries of the same data
        if (_altitudeAboveTerrLastCoord.isValid() && !qIsNaN(_altitudeAboveTerrLastRelAlt)) {
            if (_altitudeAboveTerrLastCoord.distanceTo(_coordinate) < minimumDistanceTraveled && fabs(_altitudeRelativeFact.rawValue().toFloat() - _altitudeAboveTerrLastRelAlt) < minimumAltitudeChanged ) {
                return;
            }
        }
        _altitudeAboveTerrLastCoord = _coordinate;
        _altitudeAboveTerrLastRelAlt = _altitudeRelativeFact.rawValue().toFloat();

        // If for some reason we already did a query and it hasn't arrived yet, disconnect signals and unset current query. TerrainQuery system will
        // automatically delete that forgotten query.
        if (_altitudeAboveTerrTerrainAtCoordinateQuery) {
            // qCDebug(VehicleLog) << "_updateAltAboveTerrain: cleaning previous query, no longer needed";
            disconnect(_altitudeAboveTerrTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &Vehicle::_altitudeAboveTerrainReceived);
            _altitudeAboveTerrTerrainAtCoordinateQuery = nullptr;
        }
        // Now setup and trigger the new terrain query
        _altitudeAboveTerrTerrainAtCoordinateQuery = new TerrainAtCoordinateQuery(true /* autoDelet */);
        connect(_altitudeAboveTerrTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &Vehicle::_altitudeAboveTerrainReceived);
        QList<QGeoCoordinate> rgCoord;
        rgCoord.append(_coordinate);
        _altitudeAboveTerrTerrainAtCoordinateQuery->requestData(rgCoord);
        _altitudeAboveTerrQueryTimer.restart();
    }
}

void Vehicle::_altitudeAboveTerrainReceived(bool success, QList<double> heights)
{
    if (!success) {
        qCDebug(VehicleLog) << "_altitudeAboveTerrainReceived: terrain data not available for vehicle coordinate";
    } else {
        // Query was succesful, save the data.
        double terrainAltitude = heights[0];
        double altitudeAboveTerrain = altitudeAMSL()->rawValue().toDouble() - terrainAltitude;
        _altitudeAboveTerrFact.setRawValue(altitudeAboveTerrain);
    }
    // Clean up
    _altitudeAboveTerrTerrainAtCoordinateQuery = nullptr;
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

    mavlink_msg_param_map_rc_pack_chan(static_cast<uint8_t>(_mavlink->getSystemId()),
                                       static_cast<uint8_t>(_mavlink->getComponentId()),
                                       sharedLink->mavlinkChannel(),
                                       &message,
                                       _id,
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

void Vehicle::sendJoystickDataThreadSafe(float roll, float pitch, float yaw, float thrust, quint16 buttons)
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
        buttons, 0,
        0,
        0, 0,
        0, 0, 0, 0, 0, 0
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

void Vehicle::setGripperAction(GRIPPER_ACTIONS gripperAction)
{
    sendMavCommand(
            _defaultComponentId,
            MAV_CMD_DO_GRIPPER,
            false,                               // Don't show errors
            0,                                   // Param1: Gripper ID (Always set to 0)
            gripperAction,                       // Param2: Gripper Action
            0, 0, 0, 0, 0);                      // Param 3 ~ 7 : unused
}

void Vehicle::sendGripperAction(QGCMAVLink::GRIPPER_OPTIONS gripperOption)
{
    switch(gripperOption) {
        case QGCMAVLink::Gripper_release:
            setGripperAction(GRIPPER_ACTION_RELEASE);
            break;
        case QGCMAVLink::Gripper_grab:
            setGripperAction(GRIPPER_ACTION_GRAB);
            break;
        case QGCMAVLink::Invalid_option:
            qDebug("unknown function");
            break;
        default: 
            break;
    }
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
        _mavlink->getSystemId(),
        _mavlink->getComponentId(),
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

void Vehicle::_handleMessageInterval(const mavlink_message_t& message)
{
    mavlink_message_interval_t data;
    mavlink_msg_message_interval_decode(&message, &data);

    const MavCompMsgId compMsgId = {message.compid, data.message_id};
    const int32_t rate = ( data.interval_us > 0 ) ? 1000000.0 / data.interval_us : data.interval_us;

    if(!_mavlinkMsgIntervals.contains(compMsgId) || _mavlinkMsgIntervals.value(compMsgId) != rate)
    {
        (void) _mavlinkMsgIntervals.insert(compMsgId, rate);
        emit mavlinkMsgIntervalsChanged(message.compid, data.message_id, rate);
    }
}

void Vehicle::_requestMessageMessageIntervalResultHandler(void* resultHandlerData, MAV_RESULT result, RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    if((result != MAV_RESULT_ACCEPTED) || (failureCode != RequestMessageNoFailure))
    {
        mavlink_message_interval_t data;
        mavlink_msg_message_interval_decode(&message, &data);

        Vehicle* vehicle = static_cast<Vehicle*>(resultHandlerData);
        (void) vehicle->_unsupportedMessageIds.insert(message.compid, data.message_id);
    }
}

void Vehicle::_requestMessageInterval(uint8_t compId, uint16_t msgId)
{
    if(!_unsupportedMessageIds.contains(compId, msgId))
    {
        requestMessage(
            &Vehicle::_requestMessageMessageIntervalResultHandler,
            this,
            compId,
            MAVLINK_MSG_ID_MESSAGE_INTERVAL,
            msgId
        );
    }
}

int32_t Vehicle::getMessageRate(uint8_t compId, uint16_t msgId)
{
    // TODO: Use QGCMavlinkMessage
    const MavCompMsgId compMsgId = {compId, msgId};
    int32_t rate = 0;
    if(_mavlinkMsgIntervals.contains(compMsgId))
    {
        rate = _mavlinkMsgIntervals.value(compMsgId);
    }
    else
    {
        _requestMessageInterval(compId, msgId);
    }
    return rate;
}

void Vehicle::_setMessageRateCommandResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    if((ack.result == MAV_RESULT_ACCEPTED) && (failureCode == MavCmdResultCommandResultOnly))
    {
        Vehicle* vehicle = static_cast<Vehicle*>(resultHandlerData);
        if(vehicle)
        {
            vehicle->_requestMessageInterval(compId, vehicle->_lastSetMsgIntervalMsgId);
        }
    }
}

void Vehicle::setMessageRate(uint8_t compId, uint16_t msgId, int32_t rate)
{
    const MavCmdAckHandlerInfo_t handlerInfo = {
        /* .resultHandler = */ &Vehicle::_setMessageRateCommandResultHandler,
        /* .resultHandlerData =  */ this,
        /* .progressHandler =  */ nullptr,
        /* .progressHandlerData =  */ nullptr
    };

    const float interval = (rate > 0) ? 1000000.0 / rate : rate;
    _lastSetMsgIntervalMsgId = msgId;

    sendMavCommandWithHandler(
        &handlerInfo,
        compId,
        MAV_CMD_SET_MESSAGE_INTERVAL,
        msgId,
        interval
    );
}

/*===========================================================================*/
/*                         STATUS TEXT HANDLER                               */
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
        auto eventData = _events.find(componentid);
        if (eventData != _events.end()) {
            if (eventData->data()->healthAndArmingChecksSupported()) {
                qCDebug(VehicleLog) << "Dropping preflight message (expected as event):" << text;
                return;
            }
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
    if (_isActiveVehicle) {
        qgcApp()->showCriticalVehicleMessage(message);
    }
}

/*---------------------------------------------------------------------------*/
/*===========================================================================*/
/*                                 Signing                                   */
/*===========================================================================*/

void Vehicle::sendSetupSigning()
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << Q_FUNC_INFO << "Primary Link Gone!";
        return;
    }

    const mavlink_channel_t channel = static_cast<mavlink_channel_t>(sharedLink->mavlinkChannel());

    mavlink_setup_signing_t setup_signing;

    mavlink_system_t target_system;
    target_system.sysid = id();
    target_system.compid = defaultComponentId();

    MAVLinkSigning::createSetupSigning(channel, target_system, setup_signing);

    mavlink_message_t msg;
    (void) mavlink_msg_setup_signing_encode_chan(_mavlink->getSystemId(), _mavlink->getComponentId(), channel, &msg, &setup_signing);

    // Since we don't get an ack back that the message was received send twice to try to make sure it makes it to the vehicle
    for (uint8_t i = 0; i < 2; ++i) {
        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
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
        qgcApp()->qgcImageProvider()->setImage(image, _id);
    });
}

uint32_t Vehicle::flowImageIndex() const
{
    return (_imageProtocolManager ? _imageProtocolManager->flowImageIndex() : 0);
}

/*---------------------------------------------------------------------------*/
