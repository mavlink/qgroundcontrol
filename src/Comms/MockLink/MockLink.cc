#include "MockLink.h"
#include "MAVLinkLib.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSigning.h"
#include "SecureMemory.h"
#include "MockLinkCamera.h"
#include "MockLinkFTP.h"
#include "MockLinkGimbal.h"
#include "MockLinkWorker.h"
#include "QGCLoggingCategory.h"
#include "FirmwarePlugin.h"
#include "FactMetaData.h"
#include "ParameterManager.h"
#include "AppMessages.h"
#include "QGCMath.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>
#include <QtCore/QSet>
#include <QtCore/QRandomGenerator>
#include <QtCore/QTemporaryFile>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <cmath>
#include <cstring>

QGC_LOGGING_CATEGORY(MockLinkLog, "Comms.MockLink.MockLink")
QGC_LOGGING_CATEGORY(MockLinkVerboseLog, "Comms.MockLink.MockLink:verbose")

std::atomic<int> MockLink::_nextVehicleSystemId{128};

QList<MockLink::FlightMode_t> MockLink::_availableFlightModes = {
    // Mode Name                Standard Mode               Custom Mode                         CanBeSet    adv
    { "Manual",                 0,                          PX4CustomMode::MANUAL,              true,       true },
    { "Stabilized",             0,                          PX4CustomMode::STABILIZED,          true,       true },
    { "Acro",                   0,                          PX4CustomMode::ACRO,                true,       true },
    { "Altitude",               0,                          PX4CustomMode::ALTCTL,              true,       false},
    { "Offboard",               0,                          PX4CustomMode::OFFBOARD,            true,       true },
    { "Position",               0,                          PX4CustomMode::POSCTL_POSCTL,       true,       false},
    { "Orbit",                  0,                          PX4CustomMode::POSCTL_ORBIT,        false,      true },
    { "Hold",                   0,                          PX4CustomMode::AUTO_LOITER,         true,       true },
    { "Mission",                0,                          PX4CustomMode::AUTO_MISSION,        true,       true },
    { "Return",                 0,                          PX4CustomMode::AUTO_RTL,            true,       true },
    { "Land",                   MAV_STANDARD_MODE_LAND,     PX4CustomMode::AUTO_LAND,           false,      true },
    { "Precision Landing",      0,                          PX4CustomMode::AUTO_PRECLAND,       true,       true },
    { "Takeoff",                MAV_STANDARD_MODE_TAKEOFF,  PX4CustomMode::AUTO_TAKEOFF,        false,      false},
    { "MockLink Mode",          0,                          PX4CustomMode::RATTITUDE,           true,       false},
    // Deliberately uses the reserved (deleted RTGS) AUTO sub-mode so QGC has no name for it
    { "(Mode not available)",   0,                          static_cast<uint32_t>(PX4_CUSTOM_MAIN_MODE_AUTO << 16 | (PX4_CUSTOM_SUB_MODE_AUTO_RESERVED_DO_NOT_USE << 24)), false, false},
    { "MockLink Mode (delayed)",0,                          PX4CustomMode::AUTO_FOLLOW_TARGET,  true,       false},
};

MockLink::MockLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _mockConfig(qobject_cast<const MockConfiguration*>(_config.get()))
    , _firmwareType(_mockConfig->firmwareType())
    , _vehicleType(_mockConfig->vehicleType())
    , _sendStatusText(_mockConfig->sendStatusText())
    , _enableCamera(_mockConfig->enableCamera())
    , _enableGimbal(_mockConfig->enableGimbal())
    , _failureMode(_mockConfig->failureMode())
    , _vehicleSystemId(_mockConfig->incrementVehicleId() ? _nextVehicleSystemId++ : static_cast<int>(_nextVehicleSystemId))
    , _vehicleLatitude(_defaultVehicleLatitude + ((_vehicleSystemId - 128) * 0.0001))
    , _vehicleLongitude(_defaultVehicleLongitude + ((_vehicleSystemId - 128) * 0.0001))
    , _boardVendorId(_mockConfig->boardVendorId())
    , _boardProductId(_mockConfig->boardProductId())
    , _missionItemHandler(new MockLinkMissionItemHandler(this))
    , _mockLinkCamera(_enableCamera ? new MockLinkCamera(this,
                                                         _mockConfig->cameraCaptureVideo(),
                                                         _mockConfig->cameraCaptureImage(),
                                                         _mockConfig->cameraHasModes(),
                                                         _mockConfig->cameraHasVideoStream(),
                                                         _mockConfig->cameraCanCaptureImageInVideoMode(),
                                                         _mockConfig->cameraCanCaptureVideoInImageMode(),
                                                         _mockConfig->cameraHasBasicZoom(),
                                                         _mockConfig->cameraHasTrackingPoint(),
                                                         _mockConfig->cameraHasTrackingRectangle())
                                    : nullptr)
    , _mockLinkGimbal(_enableGimbal ? new MockLinkGimbal(this,
                                                        _mockConfig->gimbalHasRollAxis(),
                                                        _mockConfig->gimbalHasPitchAxis(),
                                                        _mockConfig->gimbalHasYawAxis(),
                                                        _mockConfig->gimbalHasYawFollow(),
                                                        _mockConfig->gimbalHasYawLock(),
                                                        _mockConfig->gimbalHasRetract(),
                                                        _mockConfig->gimbalHasNeutral())
                                    : nullptr)
    , _mockLinkPX4Calibration(new MockLinkPX4Calibration(this))
    , _mockLinkFTP(new MockLinkFTP(_vehicleSystemId, _vehicleComponentId, this))
{
    qCDebug(MockLinkLog) << this;

    if (_mockConfig->startArmed()) {
        setArmed(true);
    }

    if (_mockConfig->preloadMission()) {
        _missionItemHandler->loadSimpleMultirotorMission();
    }

    // Initialize ADS-B vehicles with different starting conditions
    _adsbVehicles.reserve(_numberOfVehicles);
    for (int i = 0; i < _numberOfVehicles; ++i) {
        ADSBVehicle vehicle{};
        vehicle.angle = i * 72.0; // Different starting directions (angles 0, 72, 144, 216, 288)

        // Set initial coordinates slightly offset from the default coordinates
        const double latOffset = 0.001 * i;
        const double lonOffset = 0.001 * (i % 2 == 0 ? i : -i);
        vehicle.coordinate = QGeoCoordinate(_defaultVehicleLatitude + latOffset, _defaultVehicleLongitude + lonOffset);

        // Set a unique starting altitude for each vehicle near the home altitude
        vehicle.altitude = _defaultVehicleHomeAltitude + (i * 5);

        _adsbVehicles.append(vehicle);
    }

    (void) QObject::connect(this, &MockLink::writeBytesQueuedSignal, this, &MockLink::_writeBytesQueued, Qt::QueuedConnection);

    _loadParams();
    _runningTime.start();

    _workerThread = new QThread(this);
    _workerThread->setObjectName(QStringLiteral("Mock_%1").arg(_mockConfig->name()));
    _worker = new MockLinkWorker(this);
    _worker->moveToThread(_workerThread);
    (void) connect(_workerThread, &QThread::started, _worker, &MockLinkWorker::startWork);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);
    _workerThread->start();
}

MockLink::~MockLink()
{
    MockLink::disconnect();

    delete _mockLinkCamera;
    delete _mockLinkGimbal;
    delete _mockLinkPX4Calibration;

    if (!_logDownloadFilename.isEmpty()) {
        QFile::remove(_logDownloadFilename);
    }

    if (_workerThread) {
        _workerThread->quit();
        _workerThread->wait();
    }

    qCDebug(MockLinkLog) << this;
}

bool MockLink::_connect()
{
    if (!_connected) {
        _connected = true;
        _disconnectedEmitted = false;
        mavlink_status_t *const outgoingStatus = mavlink_get_channel_status(_outgoingMavlinkChannel);
        outgoingStatus->flags &= ~MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
        mavlink_status_t *const incomingStatus = mavlink_get_channel_status(_incomingMavlinkChannel);
        incomingStatus->flags &= ~MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
        emit connected();
    }

    return true;
}

void MockLink::disconnect()
{
    _missionItemHandler->shutdown();

    // Stop worker thread first to prevent any more messages from being sent.
    // This must happen before setting _connected = false to avoid race conditions
    // where the worker checks _connected (true), then we set it false, then the
    // worker continues sending messages to a disconnecting/destroyed vehicle.
    if (_workerThread && _workerThread->isRunning()) {
        _workerThread->quit();
        _workerThread->wait();
    }

    // signing/streams pointers alias MockLink memory — clear before emit in case the disconnected signal frees us.
    if (_outgoingMavlinkChannelIsSet()) {
        mavlink_status_t* const outgoingStatus = mavlink_get_channel_status(_outgoingMavlinkChannel);
        outgoingStatus->signing = nullptr;
        outgoingStatus->signing_streams = nullptr;
        mavlink_reset_channel_status(_outgoingMavlinkChannel);
    }

    if (_connected) {
        _connected = false;
        if (!_disconnectedEmitted.exchange(true)) {
            emit disconnected();
        }
    }
}

void MockLink::run1HzTasks()
{
    if (!_mavlinkStarted || !_connected || !mavlinkChannelIsSet()) {
        return;
    }

    if (linkConfiguration()->isHighLatency() && _highLatencyTransmissionEnabled) {
        _sendHighLatency2();
        return;
    }

    _sendVibration();
    _sendBatteryStatus();
    _sendNamedValueFloats();
    _sendSysStatus();
    _sendADSBVehicles();
    if (_vehicleType != MAV_TYPE_SUBMARINE) {
        _sendRemoteIDArmStatus();
    }
    _sendAvailableModesMonitor();

    if (_enableGimbal) {
        _mockLinkGimbal->run1HzTasks();
    }

    _sendEscInfo();
    _sendEscStatus();
    _sendRadioStatus();

    if (_enableCamera) {
        _mockLinkCamera->sendCameraHeartbeats();
    }

    if (!QGC::runningUnitTests()) {
        // Sending RC Channels during unit test breaks RC tests which does it's own RC simulation
        _sendRCChannels();
    }

    if (_sendHomePositionDelayCount > 0) {
        // We delay home position for better testing
        _sendHomePositionDelayCount--;
    } else {
        _sendHomePosition();
        // We piggy back on this delay to signal we have new standard modes available
        if (_availableModesMonitorSeqNumber == 0) {
            qCDebug(MockLinkLog) << "Bumping sequence number for available modes monitor to trigger requery of modes";
            _availableModesMonitorSeqNumber = 1;
        }
    }
}

void MockLink::run10HzTasks()
{
    if (linkConfiguration()->isHighLatency()) {
        return;
    }

    if (_mavlinkStarted && _connected && mavlinkChannelIsSet()) {
        _sendHeartBeat();
        const bool gpsDelayExpired = (_sendGPSPositionDelayCount == 0);
        if (_sendGPSPositionDelayCount > 0) {
            // We delay gps position for better testing
            _sendGPSPositionDelayCount--;
        }
        if (gpsDelayExpired || QGC::runningUnitTests()) {
            if (_vehicleType != MAV_TYPE_SUBMARINE) {
                _sendGpsRawInt();
                _sendGlobalPositionInt();
            }
            _sendExtendedSysState();
        }

        _sendAttitudeQuaternion();
        _sendAttitudeTarget();
        _sendLocalPositionNed();
        _sendPositionTargetLocalNed();

        _mockLinkPX4Calibration->run10HzTasks();

        if (_enableCamera) {
            _mockLinkCamera->run10HzTasks();
        }
    }
}

void MockLink::run500HzTasks()
{
    if (linkConfiguration()->isHighLatency()) {
        return;
    }

    if (_mavlinkStarted && _connected && mavlinkChannelIsSet()) {
        const int paramSends = QGC::runningUnitTests() ? kTestParamRequestListBatch : 1;
        for (int i = 0; i < paramSends; ++i) {
            _paramRequestListWorker();
        }
        _logDownloadWorker();
        _availableModesWorker();
        _apmCompassCalWorker();
        _apmAccelCalWorker();
    }
}

void MockLink::sendStatusTextMessages()
{
    _sendStatusTextMessages();
}

bool MockLink::_allocateMavlinkChannel()
{
    // should only be called by the LinkManager during setup
    Q_ASSERT(!_incomingMavlinkChannelIsSet());
    Q_ASSERT(!_outgoingMavlinkChannelIsSet());
    Q_ASSERT(!mavlinkChannelIsSet());

    if (!LinkInterface::_allocateMavlinkChannel()) {
        qCWarning(MockLinkLog) << "LinkInterface::_allocateMavlinkChannel failed";
        return false;
    }

    _incomingMavlinkChannel = LinkManager::instance()->allocateMavlinkChannel();
    if (!_incomingMavlinkChannelIsSet()) {
        qCWarning(MockLinkLog) << "_allocateMavlinkChannel aux failed";
        LinkInterface::_freeMavlinkChannel();
        return false;
    }

    _outgoingMavlinkChannel = LinkManager::instance()->allocateMavlinkChannel();
    if (!_outgoingMavlinkChannelIsSet()) {
        qCWarning(MockLinkLog) << "_allocateMavlinkChannel vehicle failed";
        LinkManager::instance()->freeMavlinkChannel(_incomingMavlinkChannel);
        LinkInterface::_freeMavlinkChannel();
        return false;
    }

    qCDebug(MockLinkLog) << "_allocateMavlinkChannel aux:" << _incomingMavlinkChannel << "vehicle:" << _outgoingMavlinkChannel;
    return true;
}

void MockLink::_freeMavlinkChannel()
{
    qCDebug(MockLinkLog) << "_freeMavlinkChannel aux:" << _incomingMavlinkChannel << "vehicle:" << _outgoingMavlinkChannel;
    if (!_incomingMavlinkChannelIsSet()) {
        Q_ASSERT(!_outgoingMavlinkChannelIsSet());
        Q_ASSERT(!mavlinkChannelIsSet());
        return;
    }

    if (_outgoingMavlinkChannelIsSet()) {
        // Detach our _mockSigning before the channel is freed; the bytes back this struct in MockLink memory.
        mavlink_reset_channel_status(_outgoingMavlinkChannel);
        LinkManager::instance()->freeMavlinkChannel(_outgoingMavlinkChannel);
        _outgoingMavlinkChannel = LinkManager::invalidMavlinkChannel();
    }
    mavlink_reset_channel_status(_incomingMavlinkChannel);
    LinkManager::instance()->freeMavlinkChannel(_incomingMavlinkChannel);
    _incomingMavlinkChannel = LinkManager::invalidMavlinkChannel();
    LinkInterface::_freeMavlinkChannel();
}

bool MockLink::_incomingMavlinkChannelIsSet() const
{
    return (LinkManager::invalidMavlinkChannel() != _incomingMavlinkChannel);
}

bool MockLink::_outgoingMavlinkChannelIsSet() const
{
    return (LinkManager::invalidMavlinkChannel() != _outgoingMavlinkChannel);
}

void MockLink::_loadParams()
{
    QFile paramFile;
    if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        if (_vehicleType == MAV_TYPE_FIXED_WING) {
            paramFile.setFileName(":/FirmwarePlugin/APM/Plane.OfflineEditing.params");
        } else if (_vehicleType == MAV_TYPE_SUBMARINE ) {
            paramFile.setFileName(":/FirmwarePlugin/APM/Sub.OfflineEditing.params");
        } else if (_vehicleType == MAV_TYPE_GROUND_ROVER ) {
            paramFile.setFileName(":/FirmwarePlugin/APM/Rover.OfflineEditing.params");
        } else {
            paramFile.setFileName(":/FirmwarePlugin/APM/Copter.OfflineEditing.params");
        }
    } else {
        paramFile.setFileName(":/MockLink/PX4MockLink.params");
    }

    const bool success = paramFile.open(QFile::ReadOnly);
    Q_UNUSED(success);
    Q_ASSERT(success);

    QTextStream paramStream(&paramFile);
    while (!paramStream.atEnd()) {
        const QString line = paramStream.readLine();

        if (line.startsWith("#")) {
            continue;
        }

        const QStringList paramData = line.split("\t");
        Q_ASSERT(paramData.count() == 5);

        const int compId = paramData.at(1).toInt();
        const QString paramName = paramData.at(2);
        const QString valStr = paramData.at(3);
        const uint paramType = paramData.at(4).toUInt();

        QVariant paramValue;
        switch (paramType) {
        case MAV_PARAM_TYPE_REAL32:
            paramValue = QVariant(valStr.toFloat());
            break;
        case MAV_PARAM_TYPE_UINT32:
            paramValue = QVariant(valStr.toUInt());
            break;
        case MAV_PARAM_TYPE_INT32:
            paramValue = QVariant(valStr.toInt());
            break;
        case MAV_PARAM_TYPE_UINT16:
            paramValue = QVariant((quint16)valStr.toUInt());
            break;
        case MAV_PARAM_TYPE_INT16:
            paramValue = QVariant((qint16)valStr.toInt());
            break;
        case MAV_PARAM_TYPE_UINT8:
            paramValue = QVariant((quint8)valStr.toUInt());
            break;
        case MAV_PARAM_TYPE_INT8:
            paramValue = QVariant((qint8)valStr.toUInt());
            break;
        default:
            qCCritical(MockLinkVerboseLog) << "Unknown type" << paramType;
            paramValue = QVariant(valStr.toInt());
            break;
        }

        qCDebug(MockLinkVerboseLog) << "Loading param" << paramName << paramValue;

        _mapParamName2Value[compId][paramName] = paramValue;
        _mapParamName2MavParamType[compId][paramName] = static_cast<MAV_PARAM_TYPE>(paramType);
    }
}

/// Unit test support: MAV_CMD_PREFLIGHT_STORAGE with param1=2 (as sent by
/// ParameterManager::resetAllParametersToDefaults) resets parameters to firmware defaults.
///
/// For PX4: resets all parameters to the values from Parameter.MetaData.json.
/// SYS_AUTOSTART is preserved unless setResetSysAutostartOnParamReset has been called.
///
/// For ArduPilot: resets only the calibration-indicator parameters (compass offsets and
/// accelerometer offsets) to 0.0, which is the ArduPilot firmware default for an
/// uncalibrated vehicle. This allows QGC to detect that sensor setup is required after
/// a reset, matching real ArduPilot behavior.
void MockLink::_resetParamsToDefaults()
{
    if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        // ArduPilot firmware default for calibration-indicator parameters is 0 (uncalibrated).
        // Resetting these causes QGC's APMSensorsComponent::setupComplete() to return false,
        // which is the correct post-reset state on a real vehicle.
        for (auto compIt = _mapParamName2Value.begin(); compIt != _mapParamName2Value.end(); ++compIt) {
            for (auto paramIt = compIt.value().begin(); paramIt != compIt.value().end(); ++paramIt) {
                if (kAPMCalOffsetParams.contains(paramIt.key())) {
                    paramIt.value() = QVariant(0.0f);
                }
            }
        }
        return;
    }

    if (_firmwareType != MAV_AUTOPILOT_PX4) {
        qCWarning(MockLinkLog) << "Param reset to defaults not supported for firmware type" << _firmwareType;
        return;
    }

    QFile metaDataFile(QStringLiteral(":/MockLink/Parameter.MetaData.json"));
    if (!metaDataFile.open(QFile::ReadOnly)) {
        qCWarning(MockLinkLog) << "Unable to open parameter metadata for reset" << metaDataFile.fileName();
        return;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(metaDataFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(MockLinkLog) << "Unable to parse parameter metadata for reset:" << parseError.errorString();
        return;
    }

    QHash<QString, QVariant> defaults;
    const QJsonArray parameters = doc.object().value(QStringLiteral("parameters")).toArray();
    for (const QJsonValue &parameter : parameters) {
        const QJsonObject paramObject = parameter.toObject();
        if (paramObject.contains(QStringLiteral("default"))) {
            defaults[paramObject.value(QStringLiteral("name")).toString()] = paramObject.value(QStringLiteral("default")).toVariant();
        }
    }

    for (auto compIt = _mapParamName2Value.begin(); compIt != _mapParamName2Value.end(); ++compIt) {
        const int compId = compIt.key();
        for (auto paramIt = compIt.value().begin(); paramIt != compIt.value().end(); ++paramIt) {
            const QString &paramName = paramIt.key();
            if (!_resetSysAutostartOnParamReset && (paramName == QLatin1String("SYS_AUTOSTART"))) {
                continue;
            }
            const auto defaultIt = defaults.constFind(paramName);
            if (defaultIt == defaults.constEnd()) {
                continue;
            }
            switch (_mapParamName2MavParamType[compId][paramName]) {
            case MAV_PARAM_TYPE_REAL32:
                paramIt.value() = QVariant(defaultIt->toFloat());
                break;
            case MAV_PARAM_TYPE_UINT32:
                paramIt.value() = QVariant(defaultIt->toUInt());
                break;
            case MAV_PARAM_TYPE_INT32:
                paramIt.value() = QVariant(defaultIt->toInt());
                break;
            case MAV_PARAM_TYPE_UINT16:
                paramIt.value() = QVariant(static_cast<quint16>(defaultIt->toUInt()));
                break;
            case MAV_PARAM_TYPE_INT16:
                paramIt.value() = QVariant(static_cast<qint16>(defaultIt->toInt()));
                break;
            case MAV_PARAM_TYPE_UINT8:
                paramIt.value() = QVariant(static_cast<quint8>(defaultIt->toUInt()));
                break;
            case MAV_PARAM_TYPE_INT8:
                paramIt.value() = QVariant(static_cast<qint8>(defaultIt->toInt()));
                break;
            default:
                qCWarning(MockLinkLog) << "Param reset skipped unhandled type" << _mapParamName2MavParamType[compId][paramName] << paramName;
                break;
            }
        }
    }
}

void MockLink::_sendHeartBeat()
{
    mavlink_message_t msg{};
    (void) mavlink_msg_heartbeat_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        _vehicleType,       // MAV_TYPE
        _firmwareType,      // MAV_AUTOPILOT
        _mavBaseMode,       // MAV_MODE
        _mavCustomMode,     // custom mode
        _mavState           // MAV_STATE
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendHighLatency2()
{
    qCDebug(MockLinkLog) << "Sending" << _mavCustomMode;

    union px4_custom_mode px4_cm{};
    px4_cm.data = _mavCustomMode;

    mavlink_message_t msg{};
    (void) mavlink_msg_high_latency2_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        0,                          // timestamp
        _vehicleType,               // MAV_TYPE
        _firmwareType,              // MAV_AUTOPILOT
        px4_cm.custom_mode_hl,      // custom_mode
        static_cast<int32_t>(_vehicleLatitude * 1E7),
        static_cast<int32_t>(_vehicleLongitude * 1E7),
        static_cast<int16_t>(_vehicleAltitudeAMSL),
        static_cast<int16_t>(_vehicleAltitudeAMSL),  // target_altitude,
        0,                          // heading
        0,                          // target_heading
        0,                          // target_distance
        0,                          // throttle
        0,                          // airspeed
        0,                          // airspeed_sp
        0,                          // groundspeed
        0,                          // windspeed,
        0,                          // wind_heading
        UINT8_MAX,                  // eph not known
        UINT8_MAX,                  // epv not known
        0,                          // temperature_air
        0,                          // climb_rate
        -1,                         // battery, do not use?
        0,                          // wp_num
        0,                          // failure_flags
        0, 0, 0                     // custom0, custom1, custom2
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendSysStatus()
{
    mavlink_message_t msg{};
    (void) mavlink_msg_sys_status_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        MAV_SYS_STATUS_SENSOR_GPS,  // onboard_control_sensors_present
        0,                          // onboard_control_sensors_enabled
        0,                          // onboard_control_sensors_health
        250,                        // load
        4200 * 4,                   // voltage_battery
        8000,                       // current_battery
        _battery1PctRemaining,      // battery_remaining
        0,0,0,0,0,0,0,0,0
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendBatteryStatus()
{
    if (_battery1PctRemaining > 1) {
        _battery1PctRemaining = static_cast<int8_t>(100 - (_runningTime.elapsed() / 1000));
        _battery1TimeRemaining = static_cast<double>(_batteryMaxTimeRemaining) * (static_cast<double>(_battery1PctRemaining) / 100.0);
        if (_battery1PctRemaining > 50) {
            _battery1ChargeState = MAV_BATTERY_CHARGE_STATE_OK;
        } else if (_battery1PctRemaining > 30) {
            _battery1ChargeState = MAV_BATTERY_CHARGE_STATE_LOW;
        } else if (_battery1PctRemaining > 20) {
            _battery1ChargeState = MAV_BATTERY_CHARGE_STATE_CRITICAL;
        } else {
            _battery1ChargeState = MAV_BATTERY_CHARGE_STATE_EMERGENCY;
        }
    }

    if (_battery2PctRemaining > 1) {
        _battery2PctRemaining = static_cast<int8_t>(100 - ((_runningTime.elapsed() / 1000) / 2));
        _battery2TimeRemaining = static_cast<double>(_batteryMaxTimeRemaining) * (static_cast<double>(_battery2PctRemaining) / 100.0);
        if (_battery2PctRemaining > 50) {
            _battery2ChargeState = MAV_BATTERY_CHARGE_STATE_OK;
        } else if (_battery2PctRemaining > 30) {
            _battery2ChargeState = MAV_BATTERY_CHARGE_STATE_LOW;
        } else if (_battery2PctRemaining > 20) {
            _battery2ChargeState = MAV_BATTERY_CHARGE_STATE_CRITICAL;
        } else {
            _battery2ChargeState = MAV_BATTERY_CHARGE_STATE_EMERGENCY;
        }
    }

    mavlink_message_t msg{};
    uint16_t rgVoltages[10]{};
    uint16_t rgVoltagesNone[10]{};
    uint16_t rgVoltagesExtNone[4]{};

    for (size_t i = 0; i < std::size(rgVoltages); i++) {
        rgVoltages[i] = UINT16_MAX;
        rgVoltagesNone[i] = UINT16_MAX;
    }
    rgVoltages[0] = rgVoltages[1] = rgVoltages[2] = 4200;

    (void) mavlink_msg_battery_status_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        1,                          // battery id
        MAV_BATTERY_FUNCTION_ALL,
        MAV_BATTERY_TYPE_LIPO,
        2100,                       // temp cdegC
        rgVoltages,
        600,                        // battery cA
        100,                        // current consumed mAh
        -1,                         // energy consumed not supported
        _battery1PctRemaining,
        _battery1TimeRemaining,
        _battery1ChargeState,
        rgVoltagesExtNone,
        0, // MAV_BATTERY_MODE
        0  // MAV_BATTERY_FAULT
    );
    respondWithMavlinkMessage(msg);

    (void) mavlink_msg_battery_status_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        2,                          // battery id
        MAV_BATTERY_FUNCTION_ALL,
        MAV_BATTERY_TYPE_LIPO,
        INT16_MAX,                  // temp cdegC
        rgVoltagesNone,
        600,                        // battery cA
        100,                        // current consumed mAh
        -1,                         // energy consumed not supported
        _battery2PctRemaining,
        _battery2TimeRemaining,
        _battery2ChargeState,
        rgVoltagesExtNone,
        0, // MAV_BATTERY_MODE
        0  // MAV_BATTERY_FAULT
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendNamedValueFloats()
{
    const uint32_t timeBootMs = static_cast<uint32_t>(_runningTime.elapsed());

    // Send two named float values with varying data to exercise Inspector instance separation
    const float sinVal = static_cast<float>(std::sin(static_cast<double>(timeBootMs) / 1000.0));
    const float cosVal = static_cast<float>(std::cos(static_cast<double>(timeBootMs) / 1000.0));

    // NAMED_VALUE_FLOAT.name is a fixed 10-byte field; pack_chan memcpys 10 bytes unconditionally.
    static constexpr char kSinName[10] = "sin_wave";
    static constexpr char kCosName[10] = "cos_wave";

    mavlink_message_t msg{};
    (void) mavlink_msg_named_value_float_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        timeBootMs,
        kSinName,
        sinVal
    );
    respondWithMavlinkMessage(msg);

    (void) mavlink_msg_named_value_float_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        timeBootMs,
        kCosName,
        cosVal
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendVibration()
{
    mavlink_message_t msg{};
    (void) mavlink_msg_vibration_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        0,       // time_usec
        50.5,    // vibration_x,
        10.5,    // vibration_y,
        60.0,    // vibration_z,
        1,       // clipping_0
        2,       // clipping_0
        3        // clipping_0
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::respondWithMavlinkMessage(const mavlink_message_t &msg)
{
    if (!_commLost) {
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
        const int cBuffer = mavlink_msg_to_send_buffer(buffer, &msg);
        const QByteArray bytes(reinterpret_cast<char*>(buffer), cBuffer);
        emit bytesReceived(this, bytes);
    }
}

void MockLink::_writeBytes(const QByteArray &bytes)
{
    // This prevents the responses to mavlink messages from being sent until the _writeBytes returns.
    emit writeBytesQueuedSignal(bytes);
}

void MockLink::_writeBytesQueued(const QByteArray &bytes)
{
    if (!_connected || !mavlinkChannelIsSet()) {
        qCDebug(MockLinkLog) << "Dropping queued bytes on disconnected/uninitialized mock link";
        return;
    }

    if (_inNSH) {
        _handleIncomingNSHBytes(bytes.constData(), bytes.length());
        return;
    }

    if (bytes.startsWith(QByteArrayLiteral("\r\r\r"))) {
        _inNSH = true;
        _handleIncomingNSHBytes(&bytes.constData()[3], bytes.length() - 3);
    }

    _handleIncomingMavlinkBytes(reinterpret_cast<const uint8_t*>(bytes.constData()), bytes.length());
}

void MockLink::_handleIncomingNSHBytes(const char *bytes, int cBytes)
{
    Q_UNUSED(cBytes);

    // Drop back out of NSH
    if ((cBytes == 4) && (bytes[0] == '\r') && (bytes[1] == '\r') && (bytes[2] == '\r')) {
        _inNSH  = false;
        return;
    }

    if (cBytes > 0) {
        qCDebug(MockLinkLog) << "NSH:" << bytes;
#if 0
        // MockLink not quite ready to handle this correctly yet
        if (strncmp(bytes, "sh /etc/init.d/rc.usb\n", cBytes) == 0) {
            // This is the mavlink start command
            _mavlinkStarted = true;
        }
#endif
    }
}

void MockLink::_handleIncomingMavlinkBytes(const uint8_t *bytes, int cBytes)
{
    mavlink_message_t msg{};
    mavlink_status_t comm{};

    QMutexLocker lock(&_incomingMavlinkMutex);
    for (qint64 i = 0; i < cBytes; i++) {
        const int parsed = mavlink_parse_char(_incomingMavlinkChannel, bytes[i], &msg, &comm);
        if (!parsed) {
            continue;
        }
        lock.unlock();
        _handleIncomingMavlinkMsg(msg);
        lock.relock();
    }
}

void MockLink::_updateIncomingMessageCounts(const mavlink_message_t &msg)
{
    _receivedMavlinkMessageCountMap[msg.msgid]++;
    _lastReceivedMavlinkMessageMap[msg.msgid] = msg;

    // Update command-specific counts if this is a COMMAND_LONG message
    if (msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t request{};
        mavlink_msg_command_long_decode(&msg, &request);

        _receivedMavCommandCountMap[static_cast<MAV_CMD>(request.command)]++;
        _receivedMavCommandByCompCountMap[static_cast<MAV_CMD>(request.command)][request.target_component]++;

        if (request.command == MAV_CMD_REQUEST_MESSAGE) {
            _receivedRequestMessageCountMap[static_cast<uint32_t>(request.param1)]++;
            _receivedRequestMessageByCompAndMsgCountMap[request.target_component][static_cast<int>(request.param1)]++;
        }
    } else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_INT) {
        mavlink_command_int_t request{};
        mavlink_msg_command_int_decode(&msg, &request);

        _receivedMavCommandCountMap[static_cast<MAV_CMD>(request.command)]++;
        _receivedMavCommandByCompCountMap[static_cast<MAV_CMD>(request.command)][request.target_component]++;
    }
}

void MockLink::_handleIncomingMavlinkMsg(const mavlink_message_t &msg)
{
    _updateIncomingMessageCounts(msg);

    if (_missionItemHandler->handleMavlinkMessage(msg)) {
        return;
    }

    if (_enableCamera && _mockLinkCamera->handleMavlinkMessage(msg)) {
        return;
    }

    if (_enableGimbal && _mockLinkGimbal->handleMavlinkMessage(msg)) {
        return;
    }

    switch (msg.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartBeat(msg);
        break;
    case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
        _handleParamRequestList(msg);
        break;
    case MAVLINK_MSG_ID_SET_MODE:
        _handleSetMode(msg);
        break;
    case MAVLINK_MSG_ID_PARAM_SET:
        _handleParamSet(msg);
        break;
    case MAVLINK_MSG_ID_PARAM_REQUEST_READ:
        _handleParamRequestRead(msg);
        break;
    case MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL:
        _handleFTP(msg);
        break;
    case MAVLINK_MSG_ID_COMMAND_LONG:
        _handleCommandLong(msg);
        break;
    case MAVLINK_MSG_ID_COMMAND_INT:
        _handleCommandInt(msg);
        break;
    case MAVLINK_MSG_ID_MANUAL_CONTROL:
        _handleManualControl(msg);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE:
        _handleRCChannelsOverride(msg);
        break;
    case MAVLINK_MSG_ID_LOG_REQUEST_LIST:
        _handleLogRequestList(msg);
        break;
    case MAVLINK_MSG_ID_LOG_REQUEST_DATA:
        _handleLogRequestData(msg);
        break;
    case MAVLINK_MSG_ID_PARAM_MAP_RC:
        _handleParamMapRC(msg);
        break;
    case MAVLINK_MSG_ID_SETUP_SIGNING:
        _handleSetupSigning(msg);
        break;
    case MAVLINK_MSG_ID_COMMAND_ACK:
        // GCS sends COMMAND_ACK(command=0) via nextClicked() to acknowledge each pose
        // during APM full accel calibration (the "Next" button press).
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            mavlink_command_ack_t ack{};
            mavlink_msg_command_ack_decode(&msg, &ack);
            if (ack.command == 0) {
                QMutexLocker locker(&_apmAccelCalMutex);
                if (_apmAccelCalPosIndex >= 0 && _apmAccelCalPosIndex < 6) {
                    _apmAccelCalGotAck = true;
                }
            }
        }
        break;
    default:
        break;
    }
}

void MockLink::_handleHeartBeat(const mavlink_message_t &msg)
{
    Q_UNUSED(msg);
    qCDebug(MockLinkVerboseLog) << "Heartbeat";
}

void MockLink::_handleParamMapRC(const mavlink_message_t &msg)
{
    mavlink_param_map_rc_t paramMapRC{};
    mavlink_msg_param_map_rc_decode(&msg, &paramMapRC);

    const QString paramName(QString::fromLocal8Bit(paramMapRC.param_id, static_cast<int>(strnlen(paramMapRC.param_id, MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN))));

    if (paramMapRC.param_index == -1) {
        qCDebug(MockLinkLog) << QStringLiteral("MockLink - PARAM_MAP_RC: param(%1) tuningID(%2) centerValue(%3) scale(%4) min(%5) max(%6)").arg(paramName).arg(paramMapRC.parameter_rc_channel_index).arg(paramMapRC.param_value0).arg(paramMapRC.scale).arg(paramMapRC.param_value_min).arg(paramMapRC.param_value_max);
    } else if (paramMapRC.param_index == -2) {
        qCDebug(MockLinkLog) << "MockLink - PARAM_MAP_RC: Clear tuningID" << paramMapRC.parameter_rc_channel_index;
    } else {
        qCWarning(MockLinkLog) << "MockLink - PARAM_MAP_RC: Unsupported param_index" << paramMapRC.param_index;
    }
}

void MockLink::_handleSetupSigning(const mavlink_message_t &msg)
{
    mavlink_setup_signing_t setupSigning{};
    mavlink_msg_setup_signing_decode(&msg, &setupSigning);

    if (setupSigning.target_system != _vehicleSystemId) {
        return;
    }

    // All-zero key = disable signing
    bool allZeroKey = true;
    for (const uint8_t byte : setupSigning.secret_key) {
        if (byte != 0) {
            allZeroKey = false;
            break;
        }
    }

    _signingEnabled = !allZeroKey;

    // Write C-state directly; routing through SigningController would clobber its _keyHint mid-confirmation.
    mavlink_status_t* const status = mavlink_get_channel_status(_outgoingMavlinkChannel);
    if (_signingEnabled) {
        memcpy(_mockSigning.secret_key, setupSigning.secret_key, sizeof(_mockSigning.secret_key));
        _mockSigning.link_id = _outgoingMavlinkChannel;
        _mockSigning.flags = MAVLINK_SIGNING_FLAG_SIGN_OUTGOING;
        _mockSigning.timestamp = MAVLinkSigning::currentSigningTimestampTicks();
        _mockSigning.accept_unsigned_callback = MAVLinkSigning::insecureConnectionAcceptUnsignedCallback;
        status->signing = &_mockSigning;
        status->signing_streams = &_mockSigningStreams;
    } else {
        QGC::secureZero(_mockSigning.secret_key, sizeof(_mockSigning.secret_key));
        _mockSigning.accept_unsigned_callback = nullptr;
        status->signing = nullptr;
        status->signing_streams = nullptr;
    }

    qCDebug(MockLinkLog) << "Signing" << (_signingEnabled ? "enabled" : "disabled");
}

void MockLink::_handleSetMode(const mavlink_message_t &msg)
{
    mavlink_set_mode_t request{};
    mavlink_msg_set_mode_decode(&msg, &request);

    Q_ASSERT(request.target_system == _vehicleSystemId);

    _mavBaseMode = request.base_mode;
    _mavCustomMode = request.custom_mode;
}

void MockLink::_handleManualControl(const mavlink_message_t &msg)
{
    mavlink_manual_control_t manualControl{};
    mavlink_msg_manual_control_decode(&msg, &manualControl);

    // INT16_MAX means "axis invalid/not provided" per MAVLink spec
    const auto axisStr = [](int16_t v) -> QString {
        return (v == INT16_MAX) ? QStringLiteral("invalid") : QString::number(v);
    };
    // Extension fields are only valid when the corresponding enabled_extensions bit is set
    const auto extStr = [](int16_t v, bool enabled) -> QString {
        return enabled ? QString::number(v) : QStringLiteral("disabled");
    };

    const uint8_t ext = manualControl.enabled_extensions;

    qCDebug(MockLinkVerboseLog).noquote()
        << "MANUAL_CONTROL"
        << "target:"             << manualControl.target
        << "x:"                  << axisStr(manualControl.x)
        << "y:"                  << axisStr(manualControl.y)
        << "z:"                  << axisStr(manualControl.z)
        << "r:"                  << axisStr(manualControl.r)
        << "buttons:"            << QStringLiteral("0x%1").arg(manualControl.buttons,  4, 16, QLatin1Char('0'))
        << "buttons2:"           << QStringLiteral("0x%1").arg(manualControl.buttons2, 4, 16, QLatin1Char('0'))
        << "enabled_extensions:" << QStringLiteral("0x%1").arg(ext, 2, 16, QLatin1Char('0'))
        << "s(pitch):"           << extStr(manualControl.s,    ext & (1 << 0))
        << "t(roll):"            << extStr(manualControl.t,    ext & (1 << 1))
        << "aux1:"               << extStr(manualControl.aux1, ext & (1 << 2))
        << "aux2:"               << extStr(manualControl.aux2, ext & (1 << 3))
        << "aux3:"               << extStr(manualControl.aux3, ext & (1 << 4))
        << "aux4:"               << extStr(manualControl.aux4, ext & (1 << 5))
        << "aux5:"               << extStr(manualControl.aux5, ext & (1 << 6))
        << "aux6:"               << extStr(manualControl.aux6, ext & (1 << 7));
}

void MockLink::_handleRCChannelsOverride(const mavlink_message_t &msg)
{
    mavlink_rc_channels_override_t override{};
    mavlink_msg_rc_channels_override_decode(&msg, &override);

    // Per the MAVLink spec:
    //   Channels 1-8:  UINT16_MAX = ignore (no state change), 0 = release back to RC radio
    //   Channels 9-18: UINT16_MAX or 0 = ignore,              UINT16_MAX-1 = release back to RC radio
    const uint16_t rawValues[18] = {
        override.chan1_raw,  override.chan2_raw,  override.chan3_raw,  override.chan4_raw,
        override.chan5_raw,  override.chan6_raw,  override.chan7_raw,  override.chan8_raw,
        override.chan9_raw,  override.chan10_raw, override.chan11_raw, override.chan12_raw,
        override.chan13_raw, override.chan14_raw, override.chan15_raw, override.chan16_raw,
        override.chan17_raw, override.chan18_raw,
    };

    bool anyChange = false;
    for (int i = 0; i < kRcChannelOverrideChannelCount; ++i) {
        const uint16_t raw = rawValues[i];
        const bool isExtended = (i >= 8);

        RCChannelOverride::State newState;
        if (isExtended) {
            if (raw == 0 || raw == UINT16_MAX) {
                continue; // ignore — no change to this channel's state
            } else if (raw == static_cast<uint16_t>(UINT16_MAX - 1)) {
                newState = RCChannelOverride::State::Released;
            } else {
                newState = RCChannelOverride::State::Overridden;
            }
        } else {
            if (raw == UINT16_MAX) {
                continue; // ignore — no change to this channel's state
            } else if (raw == 0) {
                newState = RCChannelOverride::State::Released;
            } else {
                newState = RCChannelOverride::State::Overridden;
            }
        }

        RCChannelOverride &ch = _rcChannelOverrides[i];
        if (ch.state == newState) {
            continue;
        }

        anyChange = true;

        const auto stateLabel = [](RCChannelOverride::State s) -> const char * {
            switch (s) {
            case RCChannelOverride::State::Ignore:      return "ignore";
            case RCChannelOverride::State::Released:    return "released";
            case RCChannelOverride::State::Overridden:  return "overridden";
            }
            return "unknown";
        };
        qCDebug(MockLinkLog).noquote() << QStringLiteral("RC_CHANNELS_OVERRIDE ch%1: %2 -> %3").arg(i + 1).arg(stateLabel(ch.state)).arg(stateLabel(newState));

        ch.state = newState;
        ch.value = (newState == RCChannelOverride::State::Overridden) ? raw : 0;
    }

    if (anyChange) {
        QStringList active;
        for (int i = 0; i < kRcChannelOverrideChannelCount; ++i) {
            if (_rcChannelOverrides[i].state == RCChannelOverride::State::Overridden) {
                active << QStringLiteral("ch%1").arg(i + 1);
            }
        }
        if (active.isEmpty()) {
            qCDebug(MockLinkLog) << "RC_CHANNELS_OVERRIDE: no channels currently overridden";
        } else {
            qCDebug(MockLinkLog).noquote() << "RC_CHANNELS_OVERRIDE active overrides:" << active.join(QStringLiteral(", "));
        }
    }

    for (int i = 0; i < kRcChannelOverrideChannelCount; ++i) {
        if (_rcChannelOverrides[i].state == RCChannelOverride::State::Overridden) {
            qCDebug(MockLinkVerboseLog).noquote() << QStringLiteral("RC_CHANNELS_OVERRIDE ch%1 value: %2").arg(i + 1).arg(_rcChannelOverrides[i].value);
        }
    }
}

void MockLink::_setParamFloatUnionIntoMap(int componentId, const QString &paramName, float paramFloat)
{
    Q_ASSERT(_mapParamName2Value.contains(componentId));
    Q_ASSERT(_mapParamName2Value[componentId].contains(paramName));
    Q_ASSERT(_mapParamName2MavParamType[componentId].contains(paramName));

    const MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[componentId][paramName];
    QVariant paramVariant;
    mavlink_param_union_t valueUnion{};
    valueUnion.param_float = paramFloat;
    switch (paramType) {
    case MAV_PARAM_TYPE_REAL32:
        paramVariant = QVariant::fromValue(valueUnion.param_float);
        break;
    case MAV_PARAM_TYPE_UINT32:
        paramVariant = QVariant::fromValue(valueUnion.param_uint32);
        break;
    case MAV_PARAM_TYPE_INT32:
        paramVariant = QVariant::fromValue(valueUnion.param_int32);
        break;
    case MAV_PARAM_TYPE_UINT16:
        paramVariant = QVariant::fromValue(valueUnion.param_uint16);
        break;
    case MAV_PARAM_TYPE_INT16:
        paramVariant = QVariant::fromValue(valueUnion.param_int16);
        break;
    case MAV_PARAM_TYPE_UINT8:
        paramVariant = QVariant::fromValue(valueUnion.param_uint8);
        break;
    case MAV_PARAM_TYPE_INT8:
        paramVariant = QVariant::fromValue(valueUnion.param_int8);
        break;
    default:
        qCCritical(MockLinkLog) << "Invalid parameter type" << paramType;
        paramVariant = QVariant::fromValue(valueUnion.param_int32);
        break;
    }

    qCDebug(MockLinkLog) << "_setParamFloatUnionIntoMap" << paramName << paramVariant;
    _mapParamName2Value[componentId][paramName] = paramVariant;
}

void MockLink::setMockParamValue(int componentId, const QString &paramName, float value)
{
    mavlink_param_union_t valueUnion{};
    valueUnion.param_float = value;
    _setParamFloatUnionIntoMap(componentId, paramName, valueUnion.param_float);
}

float MockLink::_floatUnionForParam(int componentId, const QString &paramName)
{
    Q_ASSERT(_mapParamName2Value.contains(componentId));
    Q_ASSERT(_mapParamName2Value[componentId].contains(paramName));
    Q_ASSERT(_mapParamName2MavParamType[componentId].contains(paramName));

    const MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[componentId][paramName];
    const QVariant paramVar = _mapParamName2Value[componentId][paramName];

    mavlink_param_union_t valueUnion{};
    switch (paramType) {
    case MAV_PARAM_TYPE_REAL32:
        valueUnion.param_float = paramVar.toFloat();
        break;
    case MAV_PARAM_TYPE_UINT32:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toUInt();
        } else {
            valueUnion.param_uint32 = paramVar.toUInt();
        }
        break;
    case MAV_PARAM_TYPE_INT32:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toInt();
        } else {
            valueUnion.param_int32 = paramVar.toInt();
        }
        break;
    case MAV_PARAM_TYPE_UINT16:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toUInt();
        } else {
            valueUnion.param_uint16 = paramVar.toUInt();
        }
        break;
    case MAV_PARAM_TYPE_INT16:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toInt();
        } else {
            valueUnion.param_int16 = paramVar.toInt();
        }
        break;
    case MAV_PARAM_TYPE_UINT8:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toUInt();
        } else {
            valueUnion.param_uint8 = paramVar.toUInt();
        }
        break;
    case MAV_PARAM_TYPE_INT8:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = (unsigned char)paramVar.toChar().toLatin1();
        } else {
            valueUnion.param_int8 = (unsigned char)paramVar.toChar().toLatin1();
        }
        break;
    default:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toInt();
        } else {
            valueUnion.param_int32 = paramVar.toInt();
        }
        qCCritical(MockLinkLog) << "Invalid parameter type" << paramType;
    }

    return valueUnion.param_float;
}

uint32_t MockLink::_computeParamHash(int componentId) const
{
    // Volatile parameters are excluded from the hash, matching PX4 firmware and ParameterManager::_tryCacheHashLoad
    static const QStringList volatileParams = {
        QStringLiteral("COM_FLIGHT_UUID"),
        QStringLiteral("EKF2_MAGBIAS_X"),
        QStringLiteral("EKF2_MAGBIAS_Y"),
        QStringLiteral("EKF2_MAGBIAS_Z"),
        QStringLiteral("EKF2_MAG_DECL"),
        QStringLiteral("LND_FLIGHT_T_HI"),
        QStringLiteral("LND_FLIGHT_T_LO"),
        QStringLiteral("SYS_RESTART_TYPE"),
    };

    uint32_t crc = 0;
    const auto &params = _mapParamName2Value[componentId];
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        const QString &name = it.key();
        if (volatileParams.contains(name)) {
            continue;
        }
        const QVariant &value = it.value();
        const MAV_PARAM_TYPE mavType = _mapParamName2MavParamType[componentId][name];
        const FactMetaData::ValueType_t factType = ParameterManager::mavTypeToFactType(mavType);

        crc = QGC::crc32(reinterpret_cast<const uint8_t *>(qPrintable(name)), name.length(), crc);
        crc = QGC::crc32(static_cast<const uint8_t *>(value.constData()), FactMetaData::typeToSize(factType), crc);
    }
    return crc;
}

void MockLink::_handleParamRequestList(const mavlink_message_t &msg)
{
    if (_failureMode == MockConfiguration::FailParamNoResponseToRequestList) {
        return;
    }

    mavlink_param_request_list_t request{};
    mavlink_msg_param_request_list_decode(&msg, &request);

    Q_ASSERT(request.target_system == _vehicleSystemId);
    Q_ASSERT(request.target_component == MAV_COMP_ID_ALL);

    // Cache component IDs and first component's param names to avoid repeated keys() calls in worker
    // Thread safety: Lock mutex before modifying shared state accessed by worker thread
    QMutexLocker locker(&_paramRequestListMutex);
    _paramRequestListComponentIds = _mapParamName2Value.keys();
    if (!_paramRequestListComponentIds.isEmpty()) {
        _paramRequestListParamNames = _mapParamName2Value[_paramRequestListComponentIds.first()].keys();
    }

    // Start the worker routine
    _currentParamRequestListComponentIndex = 0;
    _currentParamRequestListParamIndex = 0;
    _paramRequestListHashCheckSent = false;
}

void MockLink::_paramRequestListWorker()
{
    if (_currentParamRequestListComponentIndex == -1) {
        // Initial request complete
        return;
    }

    // Thread safety: Lock mutex before accessing shared state modified by main thread
    QMutexLocker locker(&_paramRequestListMutex);

    // Use cached lists instead of calling keys() on every iteration (500Hz)
    if (_currentParamRequestListComponentIndex >= _paramRequestListComponentIds.count()) {
        _currentParamRequestListComponentIndex = -1;
        return;
    }

    const int componentId = _paramRequestListComponentIds.at(_currentParamRequestListComponentIndex);
    const int cParameters = _paramRequestListParamNames.count();

    if (_currentParamRequestListParamIndex >= cParameters) {
        // All regular params sent — for PX4, append _HASH_CHECK as the last entry in the stream.
        // Uses param_count=0, param_index=-1 (same as standalone response) so ParameterManager
        // handles it via the _HASH_CHECK early-return path without affecting param count tracking.
        if (_firmwareType == MAV_AUTOPILOT_PX4 && !_paramRequestListHashCheckSent) {
            _paramRequestListHashCheckSent = true;

            mavlink_param_union_t valueUnion{};
            valueUnion.type = MAV_PARAM_TYPE_UINT32;
            valueUnion.param_uint32 = _computeParamHash(componentId);

            char paramId[MAVLINK_MSG_ID_PARAM_VALUE_LEN]{};
            (void) strncpy(paramId, "_HASH_CHECK", MAVLINK_MSG_ID_PARAM_VALUE_LEN);

            qCDebug(MockLinkLog) << "Sending _HASH_CHECK in PARAM_REQUEST_LIST stream" << componentId << "hash:" << valueUnion.param_uint32;

            mavlink_message_t responseMsg{};
            (void) mavlink_msg_param_value_pack_chan(
                _vehicleSystemId,
                componentId,
                _outgoingMavlinkChannel,
                &responseMsg,
                paramId,
                valueUnion.param_float,
                MAV_PARAM_TYPE_UINT32,
                0,      // param_count: 0 to avoid affecting ParameterManager's count tracking
                -1      // param_index: -1 signals this is a virtual/out-of-band parameter
            );
            respondWithMavlinkMessage(responseMsg);
            return;
        }

        // Move to next component
        if (++_currentParamRequestListComponentIndex >= _paramRequestListComponentIds.count()) {
            _currentParamRequestListComponentIndex = -1;
            _paramRequestListComponentIds.clear();
            _paramRequestListParamNames.clear();
        } else {
            // Cache param names for the new component
            _paramRequestListParamNames = _mapParamName2Value[_paramRequestListComponentIds.at(_currentParamRequestListComponentIndex)].keys();
            _currentParamRequestListParamIndex = 0;
            _paramRequestListHashCheckSent = false;
        }
        return;
    }

    const QString &paramName = _paramRequestListParamNames.at(_currentParamRequestListParamIndex);

    if (((_failureMode == MockConfiguration::FailMissingParamOnInitialRequest) || (_failureMode == MockConfiguration::FailMissingParamOnAllRequests)) && (paramName == _failParam)) {
        qCDebug(MockLinkLog) << "Skipping param send:" << paramName;
    } else {
        char paramId[MAVLINK_MSG_ID_PARAM_VALUE_LEN]{};
        mavlink_message_t responseMsg{};

        Q_ASSERT(_mapParamName2Value[componentId].contains(paramName));
        Q_ASSERT(_mapParamName2MavParamType[componentId].contains(paramName));

        const MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[componentId][paramName];

        Q_ASSERT(paramName.length() <= MAVLINK_MSG_ID_PARAM_VALUE_LEN);
        (void) strncpy(paramId, paramName.toLocal8Bit().constData(), MAVLINK_MSG_ID_PARAM_VALUE_LEN);

        qCDebug(MockLinkLog) << "Sending msg_param_value" << componentId << paramId << paramType << _mapParamName2Value[componentId][paramId];

        (void) mavlink_msg_param_value_pack_chan(
            _vehicleSystemId,
            componentId,                                   // component id
            _outgoingMavlinkChannel,
            &responseMsg,                                  // Outgoing message
            paramId,                                       // Parameter name
            _floatUnionForParam(componentId, paramName),   // Parameter value
            paramType,                                     // MAV_PARAM_TYPE
            cParameters,                                   // Total number of parameters
            _currentParamRequestListParamIndex             // Index of this parameter
        );
        respondWithMavlinkMessage(responseMsg);
    }

    // Move to next param index
    ++_currentParamRequestListParamIndex;
}

void MockLink::_handleParamSet(const mavlink_message_t &msg)
{
    mavlink_param_set_t request{};
    mavlink_msg_param_set_decode(&msg, &request);

    Q_ASSERT(request.target_system == _vehicleSystemId);
    const int componentId = request.target_component;

    // Param may not be null terminated if exactly fits
    char paramId[MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN + 1]{};
    paramId[MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN] = 0;
    (void) strncpy(paramId, request.param_id, MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN);

    qCDebug(MockLinkLog) << "_handleParamSet" << componentId << paramId << request.param_type;

    // PX4 special case: _HASH_CHECK is a virtual parameter used by ParameterManager
    // to signal cache-hit and stop parameter streaming. It is intentionally not part
    // of the normal parameter maps.
    if ((_firmwareType == MAV_AUTOPILOT_PX4) && (strncmp(paramId, "_HASH_CHECK", MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN) == 0)) {
        QMutexLocker locker(&_paramRequestListMutex);
        _currentParamRequestListComponentIndex = -1;
        _paramRequestListComponentIds.clear();
        _paramRequestListParamNames.clear();
        qCDebug(MockLinkLog) << "Received _HASH_CHECK PARAM_SET, stopping parameter stream";
        return;
    }

    Q_ASSERT(_mapParamName2Value.contains(componentId));
    Q_ASSERT(_mapParamName2MavParamType.contains(componentId));
    Q_ASSERT(_mapParamName2Value[componentId].contains(paramId));
    Q_ASSERT(request.param_type == _mapParamName2MavParamType[componentId][paramId]);

    // Apply failure behaviors before committing change.
    if (_paramSetFailureMode == FailParamSetFirstAttemptNoAck && _paramSetFailureFirstAttemptPending) {
        qCDebug(MockLinkLog) << "Param set failure: first attempt no ack" << paramId;
        _paramSetFailureFirstAttemptPending = false;
        return;
    }

    if (_paramSetFailureMode == FailParamSetNoAck) {
        qCDebug(MockLinkLog) << "Param set failure: no ack" << paramId;
        return;
    }

    if (_paramSetFailureMode == FailParamSetParamError) {
        qCDebug(MockLinkLog) << "Param set failure: PARAM_ERROR" << paramId;
        _sendParamError(componentId, paramId,
                        _mapParamName2Value[componentId].keys().indexOf(paramId),
                        MAV_PARAM_ERROR_VALUE_OUT_OF_RANGE);
        return;
    }

    // Normal success path
    _setParamFloatUnionIntoMap(componentId, paramId, request.param_value);

    mavlink_message_t responseMsg;
    mavlink_msg_param_value_pack_chan(
        _vehicleSystemId,
        componentId,                                               // component id
        _outgoingMavlinkChannel,
        &responseMsg,                                              // Outgoing message
        paramId,                                                   // Parameter name
        request.param_value,                                       // Send same value back
        request.param_type,                                        // Send same type back
        _mapParamName2Value[componentId].count(),                  // Total number of parameters
        _mapParamName2Value[componentId].keys().indexOf(paramId)   // Index of this parameter
    );
    respondWithMavlinkMessage(responseMsg);
}

void MockLink::_handleParamRequestRead(const mavlink_message_t &msg)
{
    mavlink_message_t responseMsg{};
    mavlink_param_request_read_t request{};
    mavlink_msg_param_request_read_decode(&msg, &request);

    const QString paramName(QString::fromLocal8Bit(request.param_id, static_cast<int>(strnlen(request.param_id, MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN))));
    const int componentId = request.target_component;

    // special case for magic _HASH_CHECK value (PX4 only)
    if ((_firmwareType == MAV_AUTOPILOT_PX4) && (paramName == "_HASH_CHECK")) {
        _hashCheckRequestCount++;
        if (_hashCheckNoResponse) {
            return;
        }

        const int hashComponentId = _mapParamName2Value.contains(MAV_COMP_ID_AUTOPILOT1)
            ? MAV_COMP_ID_AUTOPILOT1
            : _mapParamName2Value.keys().first();

        mavlink_param_union_t valueUnion{};
        valueUnion.type = MAV_PARAM_TYPE_UINT32;
        valueUnion.param_uint32 = _computeParamHash(hashComponentId);
        (void) mavlink_msg_param_value_pack_chan(
            _vehicleSystemId,
            hashComponentId,
            _outgoingMavlinkChannel,
            &responseMsg,
            request.param_id,
            valueUnion.param_float,
            MAV_PARAM_TYPE_UINT32,
            0,
            -1
        );
        respondWithMavlinkMessage(responseMsg);
        return;
    }

    Q_ASSERT(_mapParamName2Value.contains(componentId));

    char paramId[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN + 1]{};
    paramId[0] = 0;

    Q_ASSERT(request.target_system == _vehicleSystemId);

    if (request.param_index == -1) {
        // Request is by param name. Param may not be null terminated if exactly fits
        (void) strncpy(paramId, request.param_id, MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN);
    } else {
        // Request is by index
        Q_ASSERT(request.param_index >= 0 && request.param_index < _mapParamName2Value[componentId].count());

        const QString key = _mapParamName2Value[componentId].keys().at(request.param_index);
        Q_ASSERT(key.length() <= MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN);
        strcpy(paramId, key.toLocal8Bit().constData());
    }

    Q_ASSERT(_mapParamName2Value[componentId].contains(paramId));
    Q_ASSERT(_mapParamName2MavParamType[componentId].contains(paramId));

    if ((_failureMode == MockConfiguration::FailMissingParamOnAllRequests) && (strcmp(paramId, _failParam) == 0)) {
        qCDebug(MockLinkLog) << "Ignoring request read for " << _failParam;
        // Fail to send this param no matter what
        return;
    }

    if (_paramRequestReadFailureMode == FailParamRequestReadFirstAttemptNoResponse && _paramRequestReadFailureFirstAttemptPending) {
        qCDebug(MockLinkLog) << "Param request read failure: first attempt no response" << paramId;
        _paramRequestReadFailureFirstAttemptPending = false;
        return;
    }

    if (_paramRequestReadFailureMode == FailParamRequestReadNoResponse) {
        qCDebug(MockLinkLog) << "Param request read failure: no response" << paramId;
        return;
    }

    if (_paramRequestReadFailureMode == FailParamRequestReadParamError) {
        qCDebug(MockLinkLog) << "Param request read failure: PARAM_ERROR" << paramId;
        _sendParamError(componentId, paramId, request.param_index, MAV_PARAM_ERROR_DOES_NOT_EXIST);
        return;
    }

    (void) mavlink_msg_param_value_pack_chan(
        _vehicleSystemId,
        componentId,                                               // component id
        _outgoingMavlinkChannel,
        &responseMsg,                                              // Outgoing message
        paramId,                                                   // Parameter name
        _floatUnionForParam(componentId, paramId),                 // Parameter value
        _mapParamName2MavParamType[componentId][paramId],          // Parameter type
        _mapParamName2Value[componentId].count(),                  // Total number of parameters
        _mapParamName2Value[componentId].keys().indexOf(paramId)   // Index of this parameter
    );
    respondWithMavlinkMessage(responseMsg);
}

void MockLink::_sendParamError(int componentId, const char *paramId, int16_t paramIndex, uint8_t errorCode)
{
    mavlink_message_t responseMsg{};
    char paramIdBuf[MAVLINK_MSG_PARAM_ERROR_FIELD_PARAM_ID_LEN + 1] = {};
    (void) strncpy(paramIdBuf, paramId, MAVLINK_MSG_PARAM_ERROR_FIELD_PARAM_ID_LEN);

    (void) mavlink_msg_param_error_pack_chan(
        _vehicleSystemId,
        static_cast<uint8_t>(componentId),
        _outgoingMavlinkChannel,
        &responseMsg,
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        paramIdBuf,
        paramIndex,
        errorCode
    );
    respondWithMavlinkMessage(responseMsg);
}

void MockLink::_handleFTP(const mavlink_message_t &msg)
{
    _mockLinkFTP->mavlinkMessageReceived(msg);
}

void MockLink::_handleInProgressCommandLong(const mavlink_command_long_t &request)
{
    uint8_t commandResult = MAV_RESULT_UNSUPPORTED;

    switch (request.command) {
    case MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_ACCEPTED:
        // Test command which sends in progress messages and then acceptance ack
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_FAILED:
        // Test command which sends in progress messages and then failure ack
        commandResult = MAV_RESULT_FAILED;
        break;
    case MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK:
        // Test command which sends in progress messages and then never sends final result ack
        break;
    }

    mavlink_message_t commandAck{};
    (void) mavlink_msg_command_ack_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &commandAck,
        request.command,
        MAV_RESULT_IN_PROGRESS,
        1,  // progress
        0,  // result_param2
        0,  // target_system
        0   // target_component
    );
    respondWithMavlinkMessage(commandAck);

    if (request.command != MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK) {
        (void) mavlink_msg_command_ack_pack_chan(
            _vehicleSystemId,
            _vehicleComponentId,
            _outgoingMavlinkChannel,
            &commandAck,
            request.command,
            commandResult,
            0,  // progress
            0,  // result_param2
            0,  // target_system
            0   // target_component
        );
        respondWithMavlinkMessage(commandAck);
    }
}

void MockLink::_handleCommandLongSetMessageInterval(const mavlink_command_long_t &request, bool &accepted)
{
    // Accept only the message IDs that MAVLinkStreamConfig requests for PID tuning.
    // Anything else gets MAV_RESULT_UNSUPPORTED so unit tests will catch unexpected usage.
    static const QSet<int> kPidTuningMessageIds = {
        MAVLINK_MSG_ID_ATTITUDE_QUATERNION,
        MAVLINK_MSG_ID_ATTITUDE_TARGET,
        MAVLINK_MSG_ID_LOCAL_POSITION_NED,
        MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED,
        MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT,
        MAVLINK_MSG_ID_VFR_HUD,
    };
    accepted = kPidTuningMessageIds.contains(static_cast<int>(request.param1));
}

void MockLink::_handleCommandLong(const mavlink_message_t &msg)
{
    static bool firstCmdUser3 = true;
    static bool firstCmdUser4 = true;

    mavlink_command_long_t request{};
    mavlink_msg_command_long_decode(&msg, &request);

    uint8_t commandResult = MAV_RESULT_UNSUPPORTED;

    switch (request.command) {
    case MAV_CMD_COMPONENT_ARM_DISARM:
        if (request.param1 == 0.0f) {
            _mavBaseMode &= ~MAV_MODE_FLAG_SAFETY_ARMED;
        } else {
            _mavBaseMode |= MAV_MODE_FLAG_SAFETY_ARMED;
        }
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_PREFLIGHT_CALIBRATION:
        _handlePreFlightCalibration(request);
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_DO_MOTOR_TEST:
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_DO_START_MAG_CAL:
        // APM onboard compass calibration: start sending MAG_CAL_PROGRESS then MAG_CAL_REPORT
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            QMutexLocker locker(&_apmCompassCalMutex);
            _apmCompassCalProgress  = 0;
            _apmCompassCalTickCount = 0;
            commandResult = MAV_RESULT_ACCEPTED;
        }
        break;
    case MAV_CMD_DO_CANCEL_MAG_CAL:
        // Stop APM compass calibration worker
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            QMutexLocker locker(&_apmCompassCalMutex);
            _apmCompassCalProgress = -1;
            commandResult = MAV_RESULT_ACCEPTED;
        }
        break;
    case MAV_CMD_CONTROL_HIGH_LATENCY:
        if (linkConfiguration()->isHighLatency()) {
            _highLatencyTransmissionEnabled = static_cast<int>(request.param1) != 0;
            emit highLatencyTransmissionEnabledChanged(_highLatencyTransmissionEnabled);
            commandResult = MAV_RESULT_ACCEPTED;
        } else {
            commandResult = MAV_RESULT_FAILED;
        }
        break;
    case MAV_CMD_PREFLIGHT_STORAGE:
        if (static_cast<int>(request.param1) == 2) {
            // Reset all parameters to firmware defaults (unit test support)
            _resetParamsToDefaults();
        }
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN:
        // Unit test support: accept the reboot command so tests can exercise
        // flows which restart the vehicle (e.g. airframe Apply and Restart).
        // The actual reboot is not simulated.
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES:
        commandResult = MAV_RESULT_ACCEPTED;
        _respondWithAutopilotVersion();
        break;
    case MAV_CMD_REQUEST_MESSAGE:
    {
        bool accepted = false;
        bool noAck = false;
        _handleRequestMessage(request, accepted, noAck);
        if (noAck) {
            // FailRequestMessageCommandNoResponse: don't send any ack, let vehicle timeout
            return;
        }
        if (accepted) {
            commandResult = MAV_RESULT_ACCEPTED;
        }
        break;
    }
    case MAV_CMD_NAV_TAKEOFF:
        _handleTakeoff(request);
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED:
        // Test command which always returns MAV_RESULT_ACCEPTED
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED:
        // Test command which always returns MAV_RESULT_FAILED
        commandResult = MAV_RESULT_FAILED;
        break;
    case MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED:
        // Test command which does not respond to first request and returns MAV_RESULT_ACCEPTED on second attempt
        if (firstCmdUser3) {
            firstCmdUser3 = false;
            return;
        } else {
            firstCmdUser3 = true;
            commandResult = MAV_RESULT_ACCEPTED;
        }
        break;
    case MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED:
        // Test command which does not respond to first request and returns MAV_RESULT_FAILED on second attempt
        if (firstCmdUser4) {
            firstCmdUser4 = false;
            return;
        } else {
            firstCmdUser4 = true;
            commandResult = MAV_RESULT_FAILED;
        }
        break;
    case MAV_CMD_MOCKLINK_NO_RESPONSE:
    case MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY:
        // Test command which never responds
        return;
    case MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_ACCEPTED:
    case MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_FAILED:
    case MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK:
        _handleInProgressCommandLong(request);
        return;
    case MAV_CMD_SET_MESSAGE_INTERVAL:
    {
        bool accepted = false;

        _handleCommandLongSetMessageInterval(request, accepted);
        if (accepted) {
            commandResult = MAV_RESULT_ACCEPTED;
        }
        break;
    }
    }

    mavlink_message_t commandAck{};
    (void) mavlink_msg_command_ack_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &commandAck,
        request.command,
        commandResult,
        0,    // progress
        0,    // result_param2
        0,    // target_system
        0     // target_component
    );
    respondWithMavlinkMessage(commandAck);
}

void MockLink::_handleCommandInt(const mavlink_message_t &msg)
{
    mavlink_command_int_t request{};
    mavlink_msg_command_int_decode(&msg, &request);

    // MockLink does not implement any COMMAND_INT commands yet, so it reports them as
    // unsupported (mirroring the COMMAND_LONG default for unrecognized commands). This
    // lets unit tests exercise "try command, fall back to legacy message" code paths
    // such as Vehicle::setEstimatorOrigin.
    const uint8_t commandResult = MAV_RESULT_UNSUPPORTED;

    mavlink_message_t commandAck{};
    (void) mavlink_msg_command_ack_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &commandAck,
        request.command,
        commandResult,
        0,    // progress
        0,    // result_param2
        0,    // target_system
        0     // target_component
    );
    respondWithMavlinkMessage(commandAck);
}

void MockLink::sendUnexpectedCommandAck(MAV_CMD command, MAV_RESULT ackResult)
{
    mavlink_message_t commandAck{};
    (void) mavlink_msg_command_ack_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &commandAck,
        command,
        ackResult,
        0,    // progress
        0,    // result_param2
        0,    // target_system
        0     // target_component
    );
    respondWithMavlinkMessage(commandAck);
}

void MockLink::_respondWithAutopilotVersion()
{
    union FlightVersion {
        uint32_t raw;

        struct {
            uint8_t type;   // bits 0–7
            uint8_t patch;  // bits 8–15
            uint8_t minor;  // bits 16–23
            uint8_t major;  // bits 24–31
        } parts;

        FlightVersion(uint32_t version = 0) : raw(version) {}
    };
    FlightVersion flightVersion;

#ifndef QGC_NO_ARDUPILOT_DIALECT
    if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        flightVersion.parts.major = 4;
        flightVersion.parts.minor = 7;
        flightVersion.parts.patch = 0;
        flightVersion.parts.type = FIRMWARE_VERSION_TYPE_OFFICIAL;
    } else if (_firmwareType == MAV_AUTOPILOT_PX4) {
#endif
        flightVersion.parts.major = 1;
        flightVersion.parts.minor = 17;
        flightVersion.parts.patch = 0;
        flightVersion.parts.type = FIRMWARE_VERSION_TYPE_OFFICIAL;
#ifndef QGC_NO_ARDUPILOT_DIALECT
    }
#endif

    const uint8_t customVersion[8]{};
    const uint64_t capabilities = MAV_PROTOCOL_CAPABILITY_MAVLINK2 | MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY | MAV_PROTOCOL_CAPABILITY_MISSION_INT | ((_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) ? MAV_PROTOCOL_CAPABILITY_TERRAIN : 0);

    mavlink_message_t msg{};
    (void) mavlink_msg_autopilot_version_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        capabilities,
        flightVersion.raw,                                  // flight_sw_version,
        0,                                                  // middleware_sw_version,
        0,                                                  // os_sw_version,
        0,                                                  // board_version,
        reinterpret_cast<const uint8_t*>(&customVersion),   // flight_custom_version,
        reinterpret_cast<const uint8_t*>(&customVersion),   // middleware_custom_version,
        reinterpret_cast<const uint8_t*>(&customVersion),   // os_custom_version,
        _boardVendorId,
        _boardProductId,
        0,                                                  // uid
        0                                                   // uid2
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendHomePosition()
{
    const float bogus[4]{};

    mavlink_message_t msg{};
    (void) mavlink_msg_home_position_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        static_cast<int32_t>(_vehicleLatitude * 1E7),
        static_cast<int32_t>(_vehicleLongitude * 1E7),
        static_cast<int32_t>(_defaultVehicleHomeAltitude * 1000),
        0.0f, 0.0f, 0.0f,
        &bogus[0],
        0.0f, 0.0f, 0.0f,
        0
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendGpsRawInt()
{
    static uint64_t timeTick = 0;

    mavlink_message_t msg{};
    (void) mavlink_msg_gps_raw_int_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        timeTick++,                             // time since boot
        GPS_FIX_TYPE_3D_FIX,
        static_cast<int32_t>(_vehicleLatitude * 1E7),
        static_cast<int32_t>(_vehicleLongitude * 1E7),
        static_cast<int32_t>(_vehicleAltitudeAMSL * 1000),
        3 * 100,                                // hdop
        3 * 100,                                // vdop
        UINT16_MAX,                             // velocity not known
        UINT16_MAX,                             // course over ground not known
        8,                                      // satellites visible
        //-- Extension
        0,                                      // Altitude (above WGS84, EGM96 ellipsoid), in meters * 1000 (positive for up).
        0,                                      // Position uncertainty in meters * 1000 (positive for up).
        0,                                      // Altitude uncertainty in meters * 1000 (positive for up).
        0,                                      // Speed uncertainty in meters * 1000 (positive for up).
        0,                                      // Heading / track uncertainty in degrees * 1e5.
        65535                                   // Yaw not provided
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendGlobalPositionInt()
{
    static uint64_t timeTick = 0;

    mavlink_message_t msg{};
    (void) mavlink_msg_global_position_int_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        timeTick++, // time since boot
        static_cast<int32_t>(_vehicleLatitude * 1E7),
        static_cast<int32_t>(_vehicleLongitude * 1E7),
        static_cast<int32_t>(_vehicleAltitudeAMSL * 1000),
        static_cast<int32_t>((_vehicleAltitudeAMSL - _defaultVehicleHomeAltitude) * 1000),
        0, 0, 0,    // no speed sent
        UINT16_MAX  // no heading sent
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendAttitudeQuaternion()
{
    const uint32_t timeBootMs = static_cast<uint32_t>(_runningTime.elapsed());
    const float t = timeBootMs / 1000.0f;

    // Synthesize sinusoidal Euler angles (rad)
    const float roll  = 0.20f * std::sin(2.0f * static_cast<float>(M_PI) * 0.50f * t);
    const float pitch = 0.10f * std::sin(2.0f * static_cast<float>(M_PI) * 0.40f * t);
    const float yaw   = 0.30f * std::sin(2.0f * static_cast<float>(M_PI) * 0.10f * t);

    // ZYX Euler → quaternion
    const float cr = std::cos(roll  / 2.0f), sr = std::sin(roll  / 2.0f);
    const float cp = std::cos(pitch / 2.0f), sp = std::sin(pitch / 2.0f);
    const float cy = std::cos(yaw   / 2.0f), sy = std::sin(yaw   / 2.0f);
    const float q1 = cr * cp * cy + sr * sp * sy; // w
    const float q2 = sr * cp * cy - cr * sp * sy; // x
    const float q3 = cr * sp * cy + sr * cp * sy; // y
    const float q4 = cr * cp * sy - sr * sp * cy; // z

    // Body rates = time-derivatives of the Euler angles
    const float rollspeed  = 0.20f * (2.0f * static_cast<float>(M_PI) * 0.50f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.50f * t);
    const float pitchspeed = 0.10f * (2.0f * static_cast<float>(M_PI) * 0.40f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.40f * t);
    const float yawspeed   = 0.30f * (2.0f * static_cast<float>(M_PI) * 0.10f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.10f * t);

    const float reprOffsetQ[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // identity

    mavlink_message_t msg{};
    (void) mavlink_msg_attitude_quaternion_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        timeBootMs,
        q1, q2, q3, q4,
        rollspeed, pitchspeed, yawspeed,
        reprOffsetQ
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendAttitudeTarget()
{
    // Setpoint: same shape as actual, phase-shifted by +0.3 rad
    const uint32_t timeBootMs = static_cast<uint32_t>(_runningTime.elapsed());
    const float t = timeBootMs / 1000.0f;
    static constexpr float kPhase = 0.3f; // rad

    const float roll  = 0.20f * std::sin(2.0f * static_cast<float>(M_PI) * 0.50f * t + kPhase);
    const float pitch = 0.10f * std::sin(2.0f * static_cast<float>(M_PI) * 0.40f * t + kPhase);
    const float yaw   = 0.30f * std::sin(2.0f * static_cast<float>(M_PI) * 0.10f * t + kPhase);

    const float cr = std::cos(roll  / 2.0f), sr = std::sin(roll  / 2.0f);
    const float cp = std::cos(pitch / 2.0f), sp = std::sin(pitch / 2.0f);
    const float cy = std::cos(yaw   / 2.0f), sy = std::sin(yaw   / 2.0f);
    const float qSp[4] = {
        cr * cp * cy + sr * sp * sy,
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
    };

    const float bodyRollRate  = 0.20f * (2.0f * static_cast<float>(M_PI) * 0.50f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.50f * t + kPhase);
    const float bodyPitchRate = 0.10f * (2.0f * static_cast<float>(M_PI) * 0.40f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.40f * t + kPhase);
    const float bodyYawRate   = 0.30f * (2.0f * static_cast<float>(M_PI) * 0.10f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.10f * t + kPhase);

    mavlink_message_t msg{};
    (void) mavlink_msg_attitude_target_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        timeBootMs,
        0,   // type_mask: all fields valid
        qSp,
        bodyRollRate, bodyPitchRate, bodyYawRate,
        0.5f // thrust
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendLocalPositionNed()
{
    const uint32_t timeBootMs = static_cast<uint32_t>(_runningTime.elapsed());
    const float t = timeBootMs / 1000.0f;

    const float x  =  5.0f  * std::sin(2.0f * static_cast<float>(M_PI) * 0.08f * t);
    const float y  =  5.0f  * std::sin(2.0f * static_cast<float>(M_PI) * 0.10f * t);
    const float z  = -10.0f + 1.0f * std::sin(2.0f * static_cast<float>(M_PI) * 0.15f * t);  // negative = up in NED
    const float vx =  5.0f  * (2.0f * static_cast<float>(M_PI) * 0.08f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.08f * t);
    const float vy =  5.0f  * (2.0f * static_cast<float>(M_PI) * 0.10f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.10f * t);
    const float vz =  1.0f  * (2.0f * static_cast<float>(M_PI) * 0.15f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.15f * t);

    mavlink_message_t msg{};
    (void) mavlink_msg_local_position_ned_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        timeBootMs,
        x, y, z,
        vx, vy, vz
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendPositionTargetLocalNed()
{
    // Setpoint: same shape as _sendLocalPositionNed, with a 0.5 s time offset (not a phase in rad)
    const uint32_t timeBootMs = static_cast<uint32_t>(_runningTime.elapsed());
    const float t = timeBootMs / 1000.0f + 0.5f; // +0.5 s time lead

    const float x  =  5.0f  * std::sin(2.0f * static_cast<float>(M_PI) * 0.08f * t);
    const float y  =  5.0f  * std::sin(2.0f * static_cast<float>(M_PI) * 0.10f * t);
    const float z  = -10.0f + 1.0f * std::sin(2.0f * static_cast<float>(M_PI) * 0.15f * t);
    const float vx =  5.0f  * (2.0f * static_cast<float>(M_PI) * 0.08f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.08f * t);
    const float vy =  5.0f  * (2.0f * static_cast<float>(M_PI) * 0.10f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.10f * t);
    const float vz =  1.0f  * (2.0f * static_cast<float>(M_PI) * 0.15f) * std::cos(2.0f * static_cast<float>(M_PI) * 0.15f * t);

    mavlink_message_t msg{};
    (void) mavlink_msg_position_target_local_ned_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        timeBootMs,
        MAV_FRAME_LOCAL_NED,
        0,                              // type_mask: all fields valid
        x, y, z,
        vx, vy, vz,
        0.0f, 0.0f, 0.0f,              // acceleration not used
        0.0f, 0.0f                      // yaw, yaw_rate not used
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendExtendedSysState()
{
    mavlink_message_t msg{};
    (void) mavlink_msg_extended_sys_state_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        MAV_VTOL_STATE_UNDEFINED,
        (_vehicleAltitudeAMSL > _defaultVehicleHomeAltitude) ? MAV_LANDED_STATE_IN_AIR : MAV_LANDED_STATE_ON_GROUND
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendChunkedStatusText(uint16_t chunkId, bool missingChunks)
{
    constexpr int cChunks = 4;

    int num = 0;
    for (int i = 0; i < cChunks; i++) {
        if (missingChunks && (i & 1)) {
            continue;
        }

        int cBuf = MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN;
        char msgBuf[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN]{};

        if (i == cChunks - 1) {
            // Last chunk is partial
            cBuf /= 2;
        }

        for (int j = 0; j < cBuf - 1; j++) {
            msgBuf[j] = '0' + num++;
            if (num > 9) {
                num = 0;
            }
        }
        msgBuf[cBuf-1] = 'A' + i;

        mavlink_message_t msg{};
        (void) mavlink_msg_statustext_pack_chan(
            _vehicleSystemId,
            _vehicleComponentId,
            _outgoingMavlinkChannel,
            &msg,
            MAV_SEVERITY_INFO,
            msgBuf,
            chunkId,
            i // chunk sequence number
        );
        respondWithMavlinkMessage(msg);
    }
}

void MockLink::_sendStatusTextMessages()
{
    struct StatusMessage {
        MAV_SEVERITY severity;
        const char *msg;
    };

    static constexpr struct StatusMessage rgMessages[] = {
        { MAV_SEVERITY_INFO,        "#Testing audio output" },
        { MAV_SEVERITY_EMERGENCY,   "Status text emergency" },
        { MAV_SEVERITY_ALERT,       "Status text alert" },
        { MAV_SEVERITY_CRITICAL,    "Status text critical" },
        { MAV_SEVERITY_ERROR,       "Status text error" },
        { MAV_SEVERITY_WARNING,     "Status text warning" },
        { MAV_SEVERITY_NOTICE,      "Status text notice" },
        { MAV_SEVERITY_INFO,        "Status text info" },
        { MAV_SEVERITY_DEBUG,       "Status text debug" },
    };

    mavlink_message_t msg{};
    for (size_t i = 0; i < std::size(rgMessages); i++) {
        const struct StatusMessage *status = &rgMessages[i];
        char statusText[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN] = {};
        (void) std::strncpy(statusText, status->msg, sizeof(statusText) - 1);

        (void) mavlink_msg_statustext_pack_chan(
            _vehicleSystemId,
            _vehicleComponentId,
            _outgoingMavlinkChannel,
            &msg,
            status->severity,
            statusText,
            0, // Not a chunked sequence
            0  // Not a chunked sequence
        );
        respondWithMavlinkMessage(msg);
    }

    _sendChunkedStatusText(1, false /* missingChunks */);
    _sendChunkedStatusText(2, true /* missingChunks */);
    _sendChunkedStatusText(3, false /* missingChunks */);   // This should cause the previous incomplete chunk to spit out
    _sendChunkedStatusText(4, true /* missingChunks */);    // This should cause the timeout to fire
}

MockLink *MockLink::_startMockLink(MockConfiguration *mockConfig)
{
    mockConfig->setDynamic(true);
    SharedLinkConfigurationPtr config = LinkManager::instance()->addConfiguration(mockConfig);

    if (LinkManager::instance()->createConnectedLink(config)) {
        return qobject_cast<MockLink*>(config->link());
    }

    return nullptr;
}

MockLink *MockLink::_startMockLinkWorker(const QString &configName, MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType, bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode, bool preloadMission)
{
    MockConfiguration *const mockConfig = new MockConfiguration(configName);

    mockConfig->setFirmwareType(firmwareType);
    mockConfig->setVehicleType(vehicleType);
    mockConfig->setSendStatusText(sendStatusText);
    mockConfig->setEnableCamera(enableCamera);
    mockConfig->setEnableGimbal(enableGimbal);
    mockConfig->setFailureMode(failureMode);
    mockConfig->setPreloadMission(preloadMission);

    return _startMockLink(mockConfig);
}

MockLink *MockLink::startPX4MockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode)
{
    return _startMockLinkWorker(QStringLiteral("PX4 MultiRotor MockLink"), MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, sendStatusText, enableCamera, enableGimbal, failureMode);
}

MockLink *MockLink::startPX4MockLinkWithMission(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode)
{
    return _startMockLinkWorker(QStringLiteral("PX4 MultiRotor MockLink"), MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, sendStatusText, enableCamera, enableGimbal, failureMode, true /* preloadMission */);
}

MockLink *MockLink::startGenericMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode)
{
    return _startMockLinkWorker(QStringLiteral("Generic MockLink"), MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR, sendStatusText, enableCamera, enableGimbal, failureMode);
}

MockLink *MockLink::startNoInitialConnectMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode)
{
    return _startMockLinkWorker(QStringLiteral("No Initial Connect MockLink"), MAV_AUTOPILOT_PX4, MAV_TYPE_GENERIC, sendStatusText, enableCamera, enableGimbal, failureMode);
}

MockLink *MockLink::startAPMArduCopterMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode)
{
    return _startMockLinkWorker(QStringLiteral("ArduCopter MockLink"),MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_QUADROTOR, sendStatusText, enableCamera, enableGimbal, failureMode);
}

MockLink *MockLink::startAPMArduPlaneMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode)
{
    return _startMockLinkWorker(QStringLiteral("ArduPlane MockLink"), MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_FIXED_WING, sendStatusText, enableCamera, enableGimbal, failureMode);
}

MockLink *MockLink::startAPMArduSubMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode)
{
    return _startMockLinkWorker(QStringLiteral("ArduSub MockLink"), MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_SUBMARINE, sendStatusText, enableCamera, enableGimbal, failureMode);
}

MockLink *MockLink::startAPMArduRoverMockLink(bool sendStatusText, bool enableCamera, bool enableGimbal, MockConfiguration::FailureMode_t failureMode)
{
    return _startMockLinkWorker(QStringLiteral("ArduRover MockLink"), MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_GROUND_ROVER, sendStatusText, enableCamera, enableGimbal, failureMode);
}

void MockLink::_sendRCChannels()
{
    mavlink_message_t msg{};
    (void) mavlink_msg_rc_channels_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        0, // time_boot_ms
        16, // chancount
        1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, // channel 1-8
        1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, // channel 9-16
        UINT16_MAX, UINT16_MAX, // channel 17/18 unused
        0 // rssi
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_handlePreFlightCalibration(const mavlink_command_long_t& request)
{
    if ((request.param1 == 0) && (request.param2 == 0) && (request.param3 == 0) &&
        (request.param4 == 0) && (request.param5 == 0) && (request.param6 == 0) &&
        (request.param7 == 0)) {
        // All zeros is a calibration cancel request.
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // Send ACCELCAL_VEHICLE_POS_FAILED so controller calls _stopCalibration(Failed)
            QMutexLocker locker(&_apmAccelCalMutex);
            if (_apmAccelCalPosIndex >= 0) {
                mavlink_message_t msg{};
                mavlink_command_long_t cmd{};
                cmd.target_system    = 255;
                cmd.target_component = MAV_COMP_ID_MISSIONPLANNER;
                cmd.command          = MAV_CMD_ACCELCAL_VEHICLE_POS;
                cmd.param1           = static_cast<float>(ACCELCAL_VEHICLE_POS_FAILED);
                (void) mavlink_msg_command_long_encode_chan(
                    _vehicleSystemId, _vehicleComponentId, _outgoingMavlinkChannel, &msg, &cmd);
                respondWithMavlinkMessage(msg);
                _apmAccelCalPosIndex = -1;
            }
        } else {
            // PX4: See PX4 calibrate_cancel_check().
            (void) _mockLinkPX4Calibration->cancel();
        }
        return;
    }

    if (request.param2 == 1) {
        // Magnetometer calibration runs the full pose-driven simulation
        _mockLinkPX4Calibration->startMagCalibration();
        return;
    }

    if (request.param1 == 1) {
        sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] calibration started: 2 gyro"));
        return;
    }

    if (request.param5 == 1) {
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // APM full accelerometer calibration: drive the ACCELCAL_VEHICLE_POS handshake
            QMutexLocker locker(&_apmAccelCalMutex);
            _apmAccelCalPosIndex  = 0;
            _apmAccelCalGotAck    = false;
            _apmAccelCalTickCount = 0;
        } else {
            // Accelerometer calibration runs the full pose-driven simulation
            _mockLinkPX4Calibration->startAccelCalibration();
        }
    }
}

void MockLink::sendStatusTextMessage(uint8_t severity, const QString &text)
{
    char statusText[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN] = {};
    (void) std::strncpy(statusText, text.toUtf8().constData(), sizeof(statusText) - 1);

    mavlink_message_t msg{};
    (void) mavlink_msg_statustext_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        severity,
        statusText,
        0,
        0 // Not chunked
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_handleTakeoff(const mavlink_command_long_t &request)
{
    _vehicleAltitudeAMSL = request.param7 + _defaultVehicleHomeAltitude;
    _mavBaseMode |= MAV_MODE_FLAG_SAFETY_ARMED;
}

void MockLink::_handleLogRequestList(const mavlink_message_t &msg)
{
    mavlink_log_request_list_t request{};
    mavlink_msg_log_request_list_decode(&msg, &request);

    if ((request.start != 0) && (request.end != 0xffff)) {
        qCWarning(MockLinkLog) << "_handleLogRequestList cannot handle partial requests";
        return;
    }

    mavlink_message_t responseMsg{};
    mavlink_msg_log_entry_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &responseMsg,
        _logDownloadLogId,      // log id
        1,                      // num_logs
        1,                      // last_log_num
        0,                      // time_utc
        _logDownloadFileSize    // size
    );
    respondWithMavlinkMessage(responseMsg);
}

QString MockLink::_createRandomFile(uint32_t byteCount)
{
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        qCWarning(MockLinkLog) << "MockLink::createRandomFile open failed" << tempFile.errorString();
        return QString();
    }

    for (uint32_t bytesWritten = 0; bytesWritten < byteCount; bytesWritten++) {
        const unsigned char byte = (QRandomGenerator::global()->generate() * 0xFF) / RAND_MAX;
        (void) tempFile.write(reinterpret_cast<const char*>(&byte), 1);
    }

    tempFile.close();
    return tempFile.fileName();
}

void MockLink::_handleLogRequestData(const mavlink_message_t &msg)
{
    mavlink_log_request_data_t request{};
    mavlink_msg_log_request_data_decode(&msg, &request);

#ifdef QGC_UNITTEST_BUILD
    if (_logDownloadFilename.isEmpty()) {
        _logDownloadFilename = _createRandomFile(_logDownloadFileSize);
    }
#endif

    if (request.id != 0) {
        qCWarning(MockLinkLog) << "_handleLogRequestData id must be 0";
        return;
    }

    if (request.ofs > (_logDownloadFileSize - 1)) {
        qCWarning(MockLinkLog) << "_handleLogRequestData offset past end of file request.ofs:size" << request.ofs << _logDownloadFileSize;
        return;
    }

    // This will trigger _logDownloadWorker to send data
    // Thread-safe access: Main thread writes, worker thread reads every 2ms. Serialize to avoid
    // worker reading inconsistent offset/count or using stale values while downloading.
    QMutexLocker locker(&_logDownloadMutex);
    _logDownloadCurrentOffset = request.ofs;
    if (request.ofs + request.count > _logDownloadFileSize) {
        request.count = _logDownloadFileSize - request.ofs;
    }
    _logDownloadBytesRemaining = request.count;
}

void MockLink::_logDownloadWorker()
{
    // Runs every 2ms (500Hz on worker thread). Must protect shared state modified by main thread.
    // Without lock: main could write new offset/count while we're reading, causing corrupted downloads.
    QMutexLocker locker(&_logDownloadMutex);
    if (_logDownloadBytesRemaining == 0) {
        return;
    }

    QFile file(_logDownloadFilename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(MockLinkLog) << "_logDownloadWorker open failed" << file.errorString();
        return;
    }

    uint8_t buffer[MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN]{};

    const qint64 bytesToRead = qMin(_logDownloadBytesRemaining, (uint32_t)MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    Q_ASSERT(file.seek(_logDownloadCurrentOffset));
    Q_ASSERT(file.read(reinterpret_cast<char*>(buffer), bytesToRead) == bytesToRead);

    qCDebug(MockLinkLog) << "_logDownloadWorker" << _logDownloadCurrentOffset << _logDownloadBytesRemaining;

    mavlink_message_t responseMsg{};
    (void) mavlink_msg_log_data_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &responseMsg,
        _logDownloadLogId,
        _logDownloadCurrentOffset,
        bytesToRead,
        &buffer[0]
    );
    respondWithMavlinkMessage(responseMsg);

    _logDownloadCurrentOffset += bytesToRead;
    _logDownloadBytesRemaining -= bytesToRead;

    file.close();
}

void MockLink::_sendADSBVehicles()
{
    for (int i = 0; i < _adsbVehicles.size(); ++i) {
        // Slightly change the direction to simulate different paths
        _adsbVehicles[i].angle += (i + 1); // Vary the change to make each path unique

        // Move each vehicle by a smaller distance to simulate slower speed
        _adsbVehicles[i].coordinate = _adsbVehicles[i].coordinate.atDistanceAndAzimuth(5, _adsbVehicles[i].angle); // 50 meters per update for slower speed

        // Simulate slight variations in altitude
        _adsbVehicles[i].altitude += (i % 2 == 0 ? 0.5 : -0.5); // Increase or decrease altitude

        QByteArray callsign = QString("N12345%1").arg(i, 2, 10, QChar('0')).toLatin1();
        callsign.resize(MAVLINK_MSG_ADSB_VEHICLE_FIELD_CALLSIGN_LEN);

        // Prepare and send MAVLink message for each vehicle
        mavlink_message_t responseMsg{};
        (void) mavlink_msg_adsb_vehicle_pack_chan(
            _vehicleSystemId,
            _vehicleComponentId,
            _outgoingMavlinkChannel,
            &responseMsg,
            12345 + i, // Unique ICAO address for each vehicle
            _adsbVehicles[i].coordinate.latitude() * 1e7,
            _adsbVehicles[i].coordinate.longitude() * 1e7,
            ADSB_ALTITUDE_TYPE_GEOMETRIC,
            _adsbVehicles[i].altitude * 1000, // Altitude in millimeters
            // Use the current angle as heading
            static_cast<uint16_t>(_adsbVehicles[i].angle * 100), // Heading in centidegrees
            0, 0, // Horizontal/Vertical velocity
            callsign.constData(), // Unique callsign
            ADSB_EMITTER_TYPE_ROTOCRAFT,
            1, // Seconds since last communication
            ADSB_FLAGS_VALID_COORDS | ADSB_FLAGS_VALID_ALTITUDE | ADSB_FLAGS_VALID_HEADING | ADSB_FLAGS_VALID_CALLSIGN | ADSB_FLAGS_SIMULATED,
            0  // Squawk code
        );
        respondWithMavlinkMessage(responseMsg);
    }
}

void MockLink::_moveADSBVehicle(int vehicleIndex)
{
    _adsbAngles[vehicleIndex] += 10; // Increment angle for smoother movement
    QGeoCoordinate &coord = _adsbVehicleCoordinates[vehicleIndex];

    // Update the position based on the new angle
    coord = QGeoCoordinate(coord.latitude(), coord.longitude()).atDistanceAndAzimuth(500, _adsbAngles[vehicleIndex]);
    coord.setAltitude(100); // Keeping altitude constant for simplicity
}

void MockLink::_handleRequestMessageAutopilotVersion(const mavlink_command_long_t &/*request*/, bool &accepted)
{
    accepted = true;

    switch (_failureMode) {
    case MockConfiguration::FailNone:
        break;
    case MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure:
        accepted = false;
        return;
    case MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost:
        accepted = true;
        return;
    default:
        break;
    }

    _respondWithAutopilotVersion();
}

void MockLink::_handleRequestMessageDebug(const mavlink_command_long_t &/*request*/, bool &accepted, bool &noAck)
{
    accepted = true;
    noAck = false;

    switch (_requestMessageFailureMode) {
    case FailRequestMessageNone:
        break;
    case FailRequestMessageCommandAcceptedMsgNotSent:
        return;
    case FailRequestMessageCommandUnsupported:
        accepted = false;
        return;
    case FailRequestMessageCommandNoResponse:
        accepted = false;
        noAck = true;
        return;
    }

    mavlink_message_t responseMsg{};
    (void) mavlink_msg_debug_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &responseMsg,
        0, 0, 0
    );
    respondWithMavlinkMessage(responseMsg);
}

void MockLink::_handleRequestMessageAvailableModes(const mavlink_command_long_t &request, bool &accepted)
{
    accepted = true;

    // Thread-safe access: Check-then-set pattern must be atomic. Worker increments index every 2ms,
    // so check for "already running" and start/stop operations must serialize to prevent race where
    // main reads false, worker increments, main overwrites with different value -> lost update.
    QMutexLocker locker(&_availableModesWorkerMutex);
    if (request.param2 == 0) {
        // Request for available modes to be streamed out
        if (_availableModesWorkerNextModeIndex != 0) {
            qCWarning(MockLinkLog) << "MAVLINK_MSG_ID_AVAILABLE_MODES: _availableModesWorker already running - _availableModesWorkerNextModeIndex:" << _availableModesWorkerNextModeIndex;
            accepted = false;
            return;
        }
        qCDebug(MockLinkLog) << "MAVLINK_MSG_ID_AVAILABLE_MODES: starting available modes sequence worker";
        _availableModesWorkerNextModeIndex = 1; // Start with the first mode in sequence (1-based index)
    } else {
        // Request for specific mode
        if (request.param2 > _availableFlightModes.count()) {
            qCWarning(MockLinkLog) << "MAVLINK_MSG_ID_AVAILABLE_MODES: requested mode index out of range" << request.param2 << _availableFlightModes.count();
            accepted = false;
            return;
        }
        qCDebug(MockLinkLog) << "MAVLINK_MSG_ID_AVAILABLE_MODES: received specific mode request for index" << request.param2;
        _availableModesWorkerNextModeIndex = -request.param2; // Negative index indicates a specific single mode request
    }
}

void MockLink::_handleRequestMessage(const mavlink_command_long_t &request, bool &accepted, bool &noAck)
{
    accepted = false;
    noAck = false;

    const uint32_t requestedMessageId = static_cast<uint32_t>(request.param1);

    // Per-message-ID no-response injection: silently drop the request (no ACK, no message)
    {
        QMutexLocker locker(&_requestMessageNoResponseMutex);
        if (_requestMessageNoResponseIds.contains(requestedMessageId)) {
            noAck = true;
            return;
        }
    }

    switch (static_cast<int>(request.param1)) {
    case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
        _handleRequestMessageAutopilotVersion(request, accepted);
        break;
    case MAVLINK_MSG_ID_COMPONENT_METADATA:
        if (_firmwareType == MAV_AUTOPILOT_PX4) {
            _sendGeneralMetaData();
            accepted = true;
        }
        break;
    case MAVLINK_MSG_ID_DEBUG:
        _handleRequestMessageDebug(request, accepted, noAck);
        break;
    case MAVLINK_MSG_ID_AVAILABLE_MODES:
        _handleRequestMessageAvailableModes(request, accepted);
        break;
    }
}

void MockLink::_sendGeneralMetaData()
{
    static constexpr const char metaDataURI[MAVLINK_MSG_COMPONENT_METADATA_FIELD_URI_LEN] = "mftp://[;comp=1]general.json"; ///< "https://bit.ly/31nm0fs"

    mavlink_message_t responseMsg{};
    (void) mavlink_msg_component_metadata_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &responseMsg,
        0, // time_boot_ms
        100, // general_metadata_file_crc
        metaDataURI
    );
    respondWithMavlinkMessage(responseMsg);
}

void MockLink::setRemoteIDArmStatus(uint8_t status, const QString& error)
{
    QMutexLocker locker(&_remoteIDArmStatusMutex);
    _remoteIDArmStatus = status;
    _remoteIDArmStatusError = error;
}

void MockLink::_sendRemoteIDArmStatus()
{
    uint8_t armStatus;
    QByteArray errorUtf8;
    {
        QMutexLocker locker(&_remoteIDArmStatusMutex);
        armStatus = _remoteIDArmStatus;
        errorUtf8 = _remoteIDArmStatusError.toUtf8();
    }

    char armStatusError[MAVLINK_MSG_OPEN_DRONE_ID_ARM_STATUS_FIELD_ERROR_LEN] = {};
    std::strncpy(armStatusError, errorUtf8.constData(), sizeof(armStatusError) - 1);

    mavlink_message_t msg{};
    (void) mavlink_msg_open_drone_id_arm_status_pack(
        _vehicleSystemId,
        MAV_COMP_ID_ODID_TXRX_1,
        &msg,
        armStatus,
        armStatusError
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendEscInfo()
{
    // Send ESC_INFO for 4 motors starting at index 0.
    // count=4 and info bitmask=0x0F (all 4 online) makes the ESC indicator visible.
    static const uint16_t failureFlags[4] = {0, 0, 0, 0};
    static const uint32_t errorCount[4]   = {0, 0, 0, 0};
    static const int16_t  temperature[4]  = {3000, 3000, 3000, 3000}; // centidegrees

    mavlink_message_t msg{};
    (void) mavlink_msg_esc_info_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        0,                              // index: first group starts at 0
        static_cast<uint64_t>(_runningTime.elapsed()) * 1000, // time_usec
        0,                              // counter
        4,                              // count: 4 motors
        ESC_CONNECTION_TYPE_DSHOT,      // connection_type
        0x0F,                           // info: bitmask — motors 0-3 online
        failureFlags,
        errorCount,
        temperature
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendEscStatus()
{
    static const int32_t rpm[4]     = {5000, 5000, 5000, 5000};
    static const float   voltage[4] = {16.0f, 16.0f, 16.0f, 16.0f};
    static const float   current[4] = {5.0f, 5.0f, 5.0f, 5.0f};

    mavlink_message_t msg{};
    (void) mavlink_msg_esc_status_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        0,                              // index: first group
        static_cast<uint64_t>(_runningTime.elapsed()) * 1000, // time_usec
        rpm,
        voltage,
        current
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendRadioStatus()
{
    // Send a RADIO_STATUS message to make the TelemetryRSSI indicator visible.
    // Any non-zero rssi value triggers showIndicator (TelemetryRSSIIndicator checks lrssi.rawValue != 0).
    mavlink_message_t msg{};
    (void) mavlink_msg_radio_status_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        100,    // rssi: local signal strength
        100,    // remrssi: remote signal strength
        50,     // txbuf: transmit buffer fill (%)
        10,     // noise: local background noise
        10,     // remnoise: remote background noise
        0,      // rxerrors
        0       // fixed
    );
    respondWithMavlinkMessage(msg);
}

void MockLink::simulateConnectionRemoved()
{
    _commLost = true;
    _connectionRemoved();
}

MockLinkFTP *MockLink::mockLinkFTP() const
{
    return _mockLinkFTP;
}

void MockLink::_sendAvailableMode(uint8_t modeIndexOneBased)
{
    if (modeIndexOneBased > _availableModesCount()) {
        qCWarning(MockLinkLog) << "modeIndexOneBased out of range" << modeIndexOneBased << _availableModesCount();
        return;
    }

    qCDebug(MockLinkLog) << "_sendAvailableMode modeIndexOneBased:" << modeIndexOneBased;

    const FlightMode_t &availableMode = _availableFlightModes[modeIndexOneBased - 1];
    char modeName[MAVLINK_MSG_AVAILABLE_MODES_FIELD_MODE_NAME_LEN] = {};
    std::strncpy(modeName, availableMode.name, sizeof(modeName) - 1);

    mavlink_message_t msg{};

    (void) mavlink_msg_available_modes_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        _availableModesCount(),
        modeIndexOneBased,
        availableMode.standard_mode,
        availableMode.custom_mode,
        availableMode.canBeSet ? 0 : MAV_MODE_PROPERTY_NOT_USER_SELECTABLE,
        modeName);
    respondWithMavlinkMessage(msg);
}

void MockLink::_availableModesWorker()
{
    // Runs every 2ms (500Hz on worker thread). Reads and increments shared index modified by main.
    // Read-modify-write must be atomic to prevent lost updates or incorrect state transitions.
    QMutexLocker locker(&_availableModesWorkerMutex);
    if (_availableModesWorkerNextModeIndex == 0) {
        //  Not active
        return;
    }

    _sendAvailableMode(qAbs(_availableModesWorkerNextModeIndex));

    if (_availableModesWorkerNextModeIndex < 0) {
        // Single mode request, stop worker
        _availableModesWorkerNextModeIndex = 0;
    } else if (++_availableModesWorkerNextModeIndex > _availableModesCount()) {
        // All modes sent, stop worker
        _availableModesWorkerNextModeIndex = 0;
        qCDebug(MockLinkLog) << "_availableModesWorker: all modes sent, stopping worker";
    }
}

void MockLink::_sendAvailableModesMonitor()
{
    mavlink_message_t msg{};

    (void) mavlink_msg_available_modes_monitor_pack_chan(
        _vehicleSystemId,
        _vehicleComponentId,
        _outgoingMavlinkChannel,
        &msg,
        _availableModesMonitorSeqNumber);
    respondWithMavlinkMessage(msg);
}

int MockLink::_availableModesCount() const
{
    return _availableFlightModes.count() - (_availableModesMonitorSeqNumber == 0 ? 1 : 0); // Exclude the delayed mode
}

// ---------------------------------------------------------------------------
// ArduPilot compass calibration simulation
//
// Driven by _apmCompassCalProgress: -1 = inactive, 0..100 = current pct.
// Main thread sets it to 0 to start and -1 to stop.
// Worker sends MAG_CAL_PROGRESS at ~10Hz (every 50 calls of 500Hz worker),
// then sends MAG_CAL_REPORT when pct reaches 100.
// ---------------------------------------------------------------------------
void MockLink::_apmCompassCalWorker()
{
    if (_firmwareType != MAV_AUTOPILOT_ARDUPILOTMEGA) {
        return;
    }

    QMutexLocker locker(&_apmCompassCalMutex);
    if (_apmCompassCalProgress < 0) {
        return;
    }

    // Tick ~10 Hz: advance every 50 calls of the 500 Hz worker
    if (++_apmCompassCalTickCount < 50) {
        return;
    }
    _apmCompassCalTickCount = 0;

    const int pct = _apmCompassCalProgress;

    // Send MAG_CAL_PROGRESS for all 3 active compasses
    mavlink_message_t msg{};
    for (uint8_t id = 0; id < 3; ++id) {
        mavlink_mag_cal_progress_t progress{};
        progress.compass_id      = id;
        progress.cal_mask        = 0x07; // all 3 compasses
        progress.cal_status      = MAG_CAL_RUNNING_STEP_ONE;
        progress.completion_pct  = static_cast<uint8_t>(qMin(pct, 100));
        (void) mavlink_msg_mag_cal_progress_encode_chan(
            _vehicleSystemId, _vehicleComponentId, _outgoingMavlinkChannel, &msg, &progress);
        respondWithMavlinkMessage(msg);
    }

    if (pct >= 100) {
        // Send MAG_CAL_REPORT for all 3 compasses
        for (uint8_t id = 0; id < 3; ++id) {
            mavlink_mag_cal_report_t report{};
            report.compass_id  = id;
            report.cal_mask    = 0x07;
            report.cal_status  = MAG_CAL_SUCCESS;
            report.fitness     = 0.5f;
            (void) mavlink_msg_mag_cal_report_encode_chan(
                _vehicleSystemId, _vehicleComponentId, _outgoingMavlinkChannel, &msg, &report);
            respondWithMavlinkMessage(msg);
        }

        // Write non-zero offsets so QGC sees all compasses as calibrated after refresh.
        // _mapParamName2Value is owned by MockLink's main thread; dispatch the write there
        // to avoid a data race with concurrent param reads on the main thread.
        (void) QMetaObject::invokeMethod(this, [this] {
            constexpr int compId = MAV_COMP_ID_AUTOPILOT1;
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS_X")]   = QVariant(10.0f);
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS_Y")]   = QVariant(10.0f);
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS_Z")]   = QVariant(10.0f);
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS2_X")]  = QVariant(10.0f);
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS2_Y")]  = QVariant(10.0f);
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS2_Z")]  = QVariant(10.0f);
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS3_X")]  = QVariant(10.0f);
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS3_Y")]  = QVariant(10.0f);
            _mapParamName2Value[compId][QStringLiteral("COMPASS_OFS3_Z")]  = QVariant(10.0f);
        }, Qt::QueuedConnection);

        _apmCompassCalProgress = -1; // deactivate
    } else {
        _apmCompassCalProgress = qMin(pct + 5, 100);
    }
}

// ---------------------------------------------------------------------------
// ArduPilot full accelerometer calibration simulation
//
// The firmware sends COMMAND_LONG(MAV_CMD_ACCELCAL_VEHICLE_POS, pos) to QGC
// and waits for a COMMAND_ACK (the "Next" button press) before advancing.
// State: _apmAccelCalPosIndex -1=inactive, 0..5=current pose, 6=done.
// ---------------------------------------------------------------------------
void MockLink::_apmAccelCalWorker()
{
    if (_firmwareType != MAV_AUTOPILOT_ARDUPILOTMEGA) {
        return;
    }

    QMutexLocker locker(&_apmAccelCalMutex);
    if (_apmAccelCalPosIndex < 0) {
        return;
    }

    if (_apmAccelCalPosIndex == 6) {
        // All poses done: send SUCCESS
        mavlink_message_t msg{};
        mavlink_command_long_t cmd{};
        cmd.target_system    = 255; // GCS
        cmd.target_component = MAV_COMP_ID_MISSIONPLANNER;
        cmd.command          = MAV_CMD_ACCELCAL_VEHICLE_POS;
        cmd.param1           = static_cast<float>(ACCELCAL_VEHICLE_POS_SUCCESS);
        (void) mavlink_msg_command_long_encode_chan(
            _vehicleSystemId, _vehicleComponentId, _outgoingMavlinkChannel, &msg, &cmd);
        respondWithMavlinkMessage(msg);

        // Write non-zero accel offsets.
        // _mapParamName2Value is owned by MockLink's main thread; dispatch the write there
        // to avoid a data race with concurrent param reads on the main thread.
        (void) QMetaObject::invokeMethod(this, [this] {
            constexpr int compId = MAV_COMP_ID_AUTOPILOT1;
            _mapParamName2Value[compId][QStringLiteral("INS_ACCOFFS_X")] = QVariant(0.1f);
            _mapParamName2Value[compId][QStringLiteral("INS_ACCOFFS_Y")] = QVariant(0.1f);
            _mapParamName2Value[compId][QStringLiteral("INS_ACCOFFS_Z")] = QVariant(0.1f);
        }, Qt::QueuedConnection);

        _apmAccelCalPosIndex = -1;
        return;
    }

    // Send the current position request once; wait for Next ack before advancing
    if (!_apmAccelCalGotAck) {
        // Throttle re-sends to ~10 Hz
        if (++_apmAccelCalTickCount < 50) {
            return;
        }
        _apmAccelCalTickCount = 0;

        mavlink_message_t msg{};
        mavlink_command_long_t cmd{};
        cmd.target_system    = 255;
        cmd.target_component = MAV_COMP_ID_MISSIONPLANNER;
        cmd.command          = MAV_CMD_ACCELCAL_VEHICLE_POS;
        cmd.param1           = static_cast<float>(kAPMAccelCalPosSequence[_apmAccelCalPosIndex]);
        (void) mavlink_msg_command_long_encode_chan(
            _vehicleSystemId, _vehicleComponentId, _outgoingMavlinkChannel, &msg, &cmd);
        respondWithMavlinkMessage(msg);
    } else {
        // Ack received — advance to next pose
        _apmAccelCalGotAck  = false;
        _apmAccelCalPosIndex++;
    }
}
