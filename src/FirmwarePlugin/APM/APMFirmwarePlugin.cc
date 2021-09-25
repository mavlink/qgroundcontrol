/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMFirmwarePlugin.h"
#include "APMAutoPilotPlugin.h"
#include "QGCMAVLink.h"
#include "QGCApplication.h"
#include "APMFlightModesComponentController.h"
#include "APMAirframeComponentController.h"
#include "APMSensorsComponentController.h"
#include "APMFollowComponentController.h"
#include "APMSubMotorComponentController.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "QGCFileDownload.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "APMMavlinkStreamRateSettings.h"
#include "ArduPlaneFirmwarePlugin.h"
#include "ArduCopterFirmwarePlugin.h"
#include "ArduRoverFirmwarePlugin.h"
#include "ArduSubFirmwarePlugin.h"
#include "LinkManager.h"

#include <QTcpSocket>

QGC_LOGGING_CATEGORY(APMFirmwarePluginLog, "APMFirmwarePluginLog")

static const QRegExp APM_FRAME_REXP("^Frame: ");

const char* APMFirmwarePlugin::_artooIP =                   "10.1.1.1"; ///< IP address of ARTOO controller
const int   APMFirmwarePlugin::_artooVideoHandshakePort =   5502;       ///< Port for video handshake on ARTOO controller

/*
 * @brief APMCustomMode encapsulates the custom modes for APM
 */
APMCustomMode::APMCustomMode(uint32_t mode, bool settable) :
    _mode(mode),
    _settable(settable)
{
}

void APMCustomMode::setEnumToStringMapping(const QMap<uint32_t, QString>& enumToString)
{
    _enumToString = enumToString;
}

QString APMCustomMode::modeString() const
{
    QString mode = _enumToString.value(modeAsInt());
    if (mode.isEmpty()) {
        mode = "mode" + QString::number(modeAsInt());
    }
    return mode;
}

APMFirmwarePlugin::APMFirmwarePlugin(void)
    : _coaxialMotors(false)
{
    qmlRegisterType<APMFlightModesComponentController>  ("QGroundControl.Controllers", 1, 0, "APMFlightModesComponentController");
    qmlRegisterType<APMAirframeComponentController>     ("QGroundControl.Controllers", 1, 0, "APMAirframeComponentController");
    qmlRegisterType<APMSensorsComponentController>      ("QGroundControl.Controllers", 1, 0, "APMSensorsComponentController");
    qmlRegisterType<APMFollowComponentController>       ("QGroundControl.Controllers", 1, 0, "APMFollowComponentController");
    qmlRegisterType<APMSubMotorComponentController>     ("QGroundControl.Controllers", 1, 0, "APMSubMotorComponentController");
}

AutoPilotPlugin* APMFirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new APMAutoPilotPlugin(vehicle, vehicle);
}

bool APMFirmwarePlugin::isCapable(const Vehicle* vehicle, FirmwareCapabilities capabilities)
{
    uint32_t available = SetFlightModeCapability | PauseVehicleCapability | GuidedModeCapability;
    if (vehicle->multiRotor()) {
        available |= TakeoffVehicleCapability;
    } else if (vehicle->vtol()) {
        available |= TakeoffVehicleCapability;
    }

    return (capabilities & available) == capabilities;
}

QList<VehicleComponent*> APMFirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);

    return QList<VehicleComponent*>();
}

QStringList APMFirmwarePlugin::flightModes(Vehicle* vehicle)
{
    Q_UNUSED(vehicle)
    QStringList flightModesList;
    foreach (const APMCustomMode& customMode, _supportedModes) {
        if (customMode.canBeSet()) {
            flightModesList << customMode.modeString();
        }
    }
    return flightModesList;
}

QString APMFirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    QString flightMode = "Unknown";

    if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        foreach (const APMCustomMode& customMode, _supportedModes) {
            if (customMode.modeAsInt() == custom_mode) {
                flightMode = customMode.modeString();
            }
        }
    }
    return flightMode;
}

bool APMFirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    *base_mode = 0;
    *custom_mode = 0;

    bool found = false;

    foreach(const APMCustomMode& mode, _supportedModes) {
        if (flightMode.compare(mode.modeString(), Qt::CaseInsensitive) == 0) {
            *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
            *custom_mode = mode.modeAsInt();
            found = true;
            break;
        }
    }

    if (!found) {
        qCWarning(APMFirmwarePluginLog) << "Unknown flight Mode" << flightMode;
    }

    return found;
}

void APMFirmwarePlugin::_handleIncomingParamValue(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);

    mavlink_param_value_t paramValue;
    mavlink_param_union_t paramUnion;

    memset(&paramValue, 0, sizeof(paramValue));

    // APM stack passes all parameter values in mavlink_param_union_t.param_float no matter what
    // type they are. Fix that up to correct usage.

    mavlink_msg_param_value_decode(message, &paramValue);

    switch (paramValue.param_type) {
    case MAV_PARAM_TYPE_UINT8:
        paramUnion.param_uint8 = static_cast<uint8_t>(paramValue.param_value);
        break;
    case MAV_PARAM_TYPE_INT8:
        paramUnion.param_int8  = static_cast<int8_t>(paramValue.param_value);
        break;
    case MAV_PARAM_TYPE_UINT16:
        paramUnion.param_uint16 = static_cast<uint16_t>(paramValue.param_value);
        break;
    case MAV_PARAM_TYPE_INT16:
        paramUnion.param_int16 = static_cast<int16_t>(paramValue.param_value);
        break;
    case MAV_PARAM_TYPE_UINT32:
        paramUnion.param_uint32 = static_cast<uint32_t>(paramValue.param_value);
        break;
    case MAV_PARAM_TYPE_INT32:
        paramUnion.param_int32 = static_cast<int32_t>(paramValue.param_value);
        break;
    case MAV_PARAM_TYPE_REAL32:
        paramUnion.param_float = paramValue.param_value;
        break;
    default:
        qCCritical(APMFirmwarePluginLog) << "Invalid/Unsupported data type used in parameter:" << paramValue.param_type;
    }

    paramValue.param_value = paramUnion.param_float;

    // Re-Encoding is always done using mavlink 1.0
    uint8_t channel = _reencodeMavlinkChannel();
    QMutexLocker reencode_lock{&_reencodeMavlinkChannelMutex()};

    mavlink_status_t* mavlinkStatusReEncode = mavlink_get_channel_status(channel);
    mavlinkStatusReEncode->flags |= MAVLINK_STATUS_FLAG_IN_MAVLINK1;

    Q_ASSERT(qgcApp()->thread() == QThread::currentThread());
    mavlink_msg_param_value_encode_chan(message->sysid,
                                        message->compid,
                                        channel,
                                        message,
                                        &paramValue);
}

void APMFirmwarePlugin::_handleOutgoingParamSetThreadSafe(Vehicle* /*vehicle*/, LinkInterface* outgoingLink, mavlink_message_t* message)
{
    mavlink_param_set_t     paramSet;
    mavlink_param_union_t   paramUnion;

    memset(&paramSet, 0, sizeof(paramSet));

    // APM stack passes all parameter values in mavlink_param_union_t.param_float no matter what
    // type they are. Fix it back to the wrong way on the way out.

    mavlink_msg_param_set_decode(message, &paramSet);

    if (!_ardupilotComponentMap[paramSet.target_system][paramSet.target_component]) {
        // Message is targetted to non-ArduPilot firmware component, assume it uses current mavlink spec
        return;
    }

    paramUnion.param_float = paramSet.param_value;

    switch (paramSet.param_type) {
    case MAV_PARAM_TYPE_UINT8:
        paramSet.param_value = paramUnion.param_uint8;
        break;
    case MAV_PARAM_TYPE_INT8:
        paramSet.param_value = paramUnion.param_int8;
        break;
    case MAV_PARAM_TYPE_UINT16:
        paramSet.param_value = paramUnion.param_uint16;
        break;
    case MAV_PARAM_TYPE_INT16:
        paramSet.param_value = paramUnion.param_int16;
        break;
    case MAV_PARAM_TYPE_UINT32:
        paramSet.param_value = paramUnion.param_uint32;
        break;
    case MAV_PARAM_TYPE_INT32:
        paramSet.param_value = paramUnion.param_int32;
        break;
    case MAV_PARAM_TYPE_REAL32:
        // Already in param_float
        break;
    default:
        qCCritical(APMFirmwarePluginLog) << "Invalid/Unsupported data type used in parameter:" << paramSet.param_type;
    }

    _adjustOutgoingMavlinkMutex.lock();
    mavlink_msg_param_set_encode_chan(message->sysid,
                                      message->compid,
                                      outgoingLink->mavlinkChannel(),
                                      message,
                                      &paramSet);
    _adjustOutgoingMavlinkMutex.unlock();
}

bool APMFirmwarePlugin::_handleIncomingStatusText(Vehicle* /*vehicle*/, mavlink_message_t* message)
{
    // APM user facing calibration messages come through as high severity, we need to parse them out
    // and lower the severity on them so that they don't pop in the users face.

    QString messageText = _getMessageText(message);
    if (messageText.contains("Place vehicle") || messageText.contains("Calibration successful")) {
        _adjustCalibrationMessageSeverity(message);
        return true;
    }

    if (messageText.contains(APM_FRAME_REXP)) {
        // We need to parse the Frame: message in order to determine whether the motors are coaxial or not
        QRegExp frameTypeRegex("^Frame: (\\S*)");
        if (frameTypeRegex.indexIn(messageText) != -1) {
            QString frameType = frameTypeRegex.cap(1);
            if (!frameType.isEmpty() && (frameType == QStringLiteral("Y6") || frameType == QStringLiteral("OCTA_QUAD") || frameType == QStringLiteral("COAX"))) {
                _coaxialMotors = true;
            }
        }
    }

    return true;
}

void APMFirmwarePlugin::_handleIncomingHeartbeat(Vehicle* vehicle, mavlink_message_t* message)
{
    bool flying = false;

    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(message, &heartbeat);

    if (message->compid == MAV_COMP_ID_AUTOPILOT1) {
        // We pull Vehicle::flying state from HEARTBEAT on ArduPilot. This is a firmware specific test.
        if (vehicle->armed()) {

            flying = heartbeat.system_status == MAV_STATE_ACTIVE;
            if (!flying && vehicle->flying()) {
                // If we were previously flying, and we go into critical or emergency assume we are still flying
                flying = heartbeat.system_status == MAV_STATE_CRITICAL || heartbeat.system_status == MAV_STATE_EMERGENCY;
            }
        }
        vehicle->_setFlying(flying);
    }

    // We need to know whether this component is part of the ArduPilot stack code or not so we can adjust mavlink quirks appropriately.
    // If the component sends a heartbeat we can know that. If it's doesn't there is pretty much no way to know.
    _ardupilotComponentMap[message->sysid][message->compid] = heartbeat.autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA;

    // Force the ESP8266 to be non-ArduPilot code (it doesn't send heartbeats)
    _ardupilotComponentMap[message->sysid][MAV_COMP_ID_UDP_BRIDGE] = false;
}

bool APMFirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    // We use loss of BATTERY_STATUS/HOME_POSITION as a trigger to reinitialize stream rates
    auto instanceData = qobject_cast<APMFirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());

    if (message->msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        // We need to look at all heartbeats that go by from any component
        _handleIncomingHeartbeat(vehicle, message);
    } else if (message->msgid == MAVLINK_MSG_ID_BATTERY_STATUS && instanceData)  {
        instanceData->lastBatteryStatusTime = QTime::currentTime();
    } else if (message->msgid == MAVLINK_MSG_ID_HOME_POSITION && instanceData)  {
        instanceData->lastHomePositionTime = QTime::currentTime();
    } else {
        // Only translate messages which come from ArduPilot code. All other components are expected to follow current mavlink spec.
        if (_ardupilotComponentMap[vehicle->id()][message->compid]) {
            switch (message->msgid) {
            case MAVLINK_MSG_ID_PARAM_VALUE:
                _handleIncomingParamValue(vehicle, message);
                break;
            case MAVLINK_MSG_ID_STATUSTEXT:
                return _handleIncomingStatusText(vehicle, message);
            case MAVLINK_MSG_ID_RC_CHANNELS:
                _handleRCChannels(vehicle, message);
                break;
            case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
                _handleRCChannelsRaw(vehicle, message);
                break;
            }
        }
    }

    // If we lose BATTERY_STATUS/HOME_POSITION for reinitStreamsTimeoutSecs seconds we re-initialize stream rates
    const int reinitStreamsTimeoutSecs = 10;
    if (instanceData && (instanceData->lastBatteryStatusTime.secsTo(QTime::currentTime()) > reinitStreamsTimeoutSecs || instanceData->lastHomePositionTime.secsTo(QTime::currentTime()) > reinitStreamsTimeoutSecs)) {
        initializeStreamRates(vehicle);
    }

    return true;
}

void APMFirmwarePlugin::adjustOutgoingMavlinkMessageThreadSafe(Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message)
{
    switch (message->msgid) {
    case MAVLINK_MSG_ID_PARAM_SET:
        _handleOutgoingParamSetThreadSafe(vehicle, outgoingLink, message);
        break;
    }
}

QString APMFirmwarePlugin::_getMessageText(mavlink_message_t* message) const
{
    QByteArray b;

    b.resize(MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN+1);
    mavlink_msg_statustext_get_text(message, b.data());

    // Ensure NUL-termination
    b[b.length()-1] = '\0';
    return QString(b);
}

void APMFirmwarePlugin::_setInfoSeverity(mavlink_message_t* message) const
{
    // Re-Encoding is always done using mavlink 1.0
    uint8_t channel = _reencodeMavlinkChannel();
    QMutexLocker reencode_lock{&_reencodeMavlinkChannelMutex()};
    mavlink_status_t* mavlinkStatusReEncode = mavlink_get_channel_status(channel);
    mavlinkStatusReEncode->flags |= MAVLINK_STATUS_FLAG_IN_MAVLINK1;

    mavlink_statustext_t statusText;
    mavlink_msg_statustext_decode(message, &statusText);

    statusText.severity = MAV_SEVERITY_INFO;

    Q_ASSERT(qgcApp()->thread() == QThread::currentThread());
    mavlink_msg_statustext_encode_chan(message->sysid,
                                       message->compid,
                                       channel,
                                       message,
                                       &statusText);
}

void APMFirmwarePlugin::_adjustCalibrationMessageSeverity(mavlink_message_t* message) const
{
    mavlink_statustext_t statusText;
    mavlink_msg_statustext_decode(message, &statusText);

    // Re-Encoding is always done using mavlink 1.0
    uint8_t channel = _reencodeMavlinkChannel();
    QMutexLocker reencode_lock{&_reencodeMavlinkChannelMutex()};

    mavlink_status_t* mavlinkStatusReEncode = mavlink_get_channel_status(channel);
    mavlinkStatusReEncode->flags |= MAVLINK_STATUS_FLAG_IN_MAVLINK1;
    statusText.severity = MAV_SEVERITY_INFO;

    Q_ASSERT(qgcApp()->thread() == QThread::currentThread());
    mavlink_msg_statustext_encode_chan(message->sysid,
                                       message->compid,
                                       channel,
                                       message,
                                       &statusText);
}

void APMFirmwarePlugin::initializeStreamRates(Vehicle* vehicle)
{
    // We use loss of BATTERY_STATUS/HOME_POSITION as a trigger to reinitialize stream rates
    auto instanceData = qobject_cast<APMFirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData) {
        instanceData = new APMFirmwarePluginInstanceData(vehicle);
        instanceData->lastBatteryStatusTime = instanceData->lastHomePositionTime = QTime::currentTime();
        vehicle->setFirmwarePluginInstanceData(instanceData);
    }

    if (qgcApp()->toolbox()->settingsManager()->appSettings()->apmStartMavlinkStreams()->rawValue().toBool()) {

        APMMavlinkStreamRateSettings* streamRates = qgcApp()->toolbox()->settingsManager()->apmMavlinkStreamRateSettings();

        struct StreamInfo_s {
            MAV_DATA_STREAM mavStream;
            int             streamRate;
        };

        StreamInfo_s rgStreamInfo[] = {
            { MAV_DATA_STREAM_RAW_SENSORS,      streamRates->streamRateRawSensors()->rawValue().toInt() },
            { MAV_DATA_STREAM_EXTENDED_STATUS,  streamRates->streamRateExtendedStatus()->rawValue().toInt() },
            { MAV_DATA_STREAM_RC_CHANNELS,      streamRates->streamRateRCChannels()->rawValue().toInt() },
            { MAV_DATA_STREAM_POSITION,         streamRates->streamRatePosition()->rawValue().toInt() },
            { MAV_DATA_STREAM_EXTRA1,           streamRates->streamRateExtra1()->rawValue().toInt() },
            { MAV_DATA_STREAM_EXTRA2,           streamRates->streamRateExtra2()->rawValue().toInt() },
            { MAV_DATA_STREAM_EXTRA3,           streamRates->streamRateExtra3()->rawValue().toInt() },
        };

        for (size_t i=0; i<sizeof(rgStreamInfo)/sizeof(rgStreamInfo[0]); i++) {
            const StreamInfo_s& streamInfo = rgStreamInfo[i];

            if (streamInfo.streamRate >= 0) {
                vehicle->requestDataStream(streamInfo.mavStream, static_cast<uint16_t>(streamInfo.streamRate));
            }
        }
    }

    // ArduPilot only sends home position on first boot and then when it arms. It does not stream the position.
    // This means that depending on when QGC connects to the vehicle it may not have home position.
    // This can cause various features to not be available. So we request home position streaming ourselves.
    // The MAV_CMD_SET_MESSAGE_INTERVAL command is only supported on newer firmwares. So we set showError=false.
    // Which also means than on older firmwares you may be left with some missing features.
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL, false /* showError */, MAVLINK_MSG_ID_HOME_POSITION, 1000000 /* 1 second interval in usec */);

    instanceData->lastBatteryStatusTime = instanceData->lastHomePositionTime = QTime::currentTime();
}


void APMFirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    if (vehicle->isOfflineEditingVehicle()) {
        switch (vehicle->vehicleType()) {
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
            vehicle->setFirmwareVersion(3, 6, 0);
            break;
        case MAV_TYPE_VTOL_DUOROTOR:
        case MAV_TYPE_VTOL_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_RESERVED2:
        case MAV_TYPE_VTOL_RESERVED3:
        case MAV_TYPE_VTOL_RESERVED4:
        case MAV_TYPE_VTOL_RESERVED5:
        case MAV_TYPE_FIXED_WING:
            vehicle->setFirmwareVersion(3, 9, 0);
            break;
        case MAV_TYPE_GROUND_ROVER:
        case MAV_TYPE_SURFACE_BOAT:
            vehicle->setFirmwareVersion(3, 5, 0);
            break;
        case MAV_TYPE_SUBMARINE:
            vehicle->setFirmwareVersion(3, 4, 0);
            break;
        default:
            // No version set
            break;
        }
    } else {
        initializeStreamRates(vehicle);
    }

    if (qgcApp()->toolbox()->settingsManager()->videoSettings()->videoSource()->rawValue() == VideoSettings::videoSource3DRSolo) {
        _soloVideoHandshake();
    }
}

void APMFirmwarePlugin::setSupportedModes(QList<APMCustomMode> supportedModes)
{
    _supportedModes = supportedModes;
}

bool APMFirmwarePlugin::sendHomePositionToVehicle(void)
{
    // APM stack wants the home position sent in the first position
    return true;
}

FactMetaData* APMFirmwarePlugin::_getMetaDataForFact(QObject* parameterMetaData, const QString& name, FactMetaData::ValueType_t type, MAV_TYPE vehicleType)
{
    APMParameterMetaData* apmMetaData = qobject_cast<APMParameterMetaData*>(parameterMetaData);

    if (apmMetaData) {
        return apmMetaData->getMetaDataForFact(name, vehicleType, type);
    } else {
        qWarning() << "Internal error: pointer passed to APMFirmwarePlugin::addMetaDataToFact not APMParameterMetaData";
    }

    return nullptr;
}

QList<MAV_CMD> APMFirmwarePlugin::supportedMissionCommands(QGCMAVLink::VehicleClass_t vehicleClass)
{
    QList<MAV_CMD> supportedCommands = {
        MAV_CMD_NAV_WAYPOINT,
        MAV_CMD_NAV_LOITER_UNLIM, MAV_CMD_NAV_LOITER_TURNS, MAV_CMD_NAV_LOITER_TIME,
        MAV_CMD_NAV_RETURN_TO_LAUNCH,
        MAV_CMD_NAV_CONTINUE_AND_CHANGE_ALT,
        MAV_CMD_NAV_LOITER_TO_ALT,
        MAV_CMD_NAV_SPLINE_WAYPOINT,
        MAV_CMD_NAV_GUIDED_ENABLE,
        MAV_CMD_NAV_DELAY,
        MAV_CMD_CONDITION_DELAY, MAV_CMD_CONDITION_DISTANCE, MAV_CMD_CONDITION_YAW,
        MAV_CMD_DO_SET_MODE,
        MAV_CMD_DO_JUMP,
        MAV_CMD_DO_CHANGE_SPEED,
        MAV_CMD_DO_SET_HOME,
        MAV_CMD_DO_SET_RELAY, MAV_CMD_DO_REPEAT_RELAY,
        MAV_CMD_DO_SET_SERVO, MAV_CMD_DO_REPEAT_SERVO,
        MAV_CMD_DO_LAND_START,
        MAV_CMD_DO_SET_ROI,
        MAV_CMD_DO_DIGICAM_CONFIGURE, MAV_CMD_DO_DIGICAM_CONTROL,
        MAV_CMD_DO_MOUNT_CONTROL,
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_DO_FENCE_ENABLE,
        MAV_CMD_DO_PARACHUTE,
        MAV_CMD_DO_INVERTED_FLIGHT,
        MAV_CMD_DO_GRIPPER,
        MAV_CMD_DO_GUIDED_LIMITS,
        MAV_CMD_DO_AUTOTUNE_ENABLE,
    };

    QList<MAV_CMD> vtolCommands = {
        MAV_CMD_NAV_VTOL_TAKEOFF, MAV_CMD_NAV_VTOL_LAND, MAV_CMD_DO_VTOL_TRANSITION,
    };

    QList<MAV_CMD> flightCommands = {
        MAV_CMD_NAV_LAND, MAV_CMD_NAV_TAKEOFF,
    };

    if (vehicleClass == QGCMAVLink::VehicleClassGeneric) {
        supportedCommands   += vtolCommands;
        supportedCommands   += flightCommands;
    }
    if (vehicleClass == QGCMAVLink::VehicleClassVTOL) {
        supportedCommands += vtolCommands;
        supportedCommands += flightCommands;
    } else if (vehicleClass == QGCMAVLink::VehicleClassFixedWing || vehicleClass == QGCMAVLink::VehicleClassMultiRotor) {
        supportedCommands += flightCommands;
    }

    if (qgcApp()->toolbox()->settingsManager()->planViewSettings()->useConditionGate()->rawValue().toBool()) {
        supportedCommands.append(MAV_CMD_CONDITION_GATE);
    }

    return supportedCommands;
}

QString APMFirmwarePlugin::missionCommandOverrides(QGCMAVLink::VehicleClass_t vehicleClass) const
{
    switch (vehicleClass) {
    case QGCMAVLink::VehicleClassGeneric:
        return QStringLiteral(":/json/APM-MavCmdInfoCommon.json");
    case QGCMAVLink::VehicleClassFixedWing:
        return QStringLiteral(":/json/APM-MavCmdInfoFixedWing.json");
    case QGCMAVLink::VehicleClassMultiRotor:
        return QStringLiteral(":/json/APM-MavCmdInfoMultiRotor.json");
    case QGCMAVLink::VehicleClassVTOL:
        return QStringLiteral(":/json/APM-MavCmdInfoVTOL.json");
    case QGCMAVLink::VehicleClassSub:
        return QStringLiteral(":/json/APM-MavCmdInfoSub.json");
    case QGCMAVLink::VehicleClassRoverBoat:
        return QStringLiteral(":/json/APM-MavCmdInfoRover.json");
    default:
        qWarning() << "APMFirmwarePlugin::missionCommandOverrides called with bad VehicleClass_t:" << vehicleClass;
        return QString();
    }
}

QObject* APMFirmwarePlugin::_loadParameterMetaData(const QString& metaDataFile)
{
    Q_UNUSED(metaDataFile);

    APMParameterMetaData* metaData = new APMParameterMetaData();
    metaData->loadParameterFactMetaDataFile(metaDataFile);
    return metaData;
}

QString APMFirmwarePlugin::getHobbsMeter(Vehicle* vehicle) 
{
    uint64_t hobbsTimeSeconds = 0;

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "STAT_FLTTIME")) {
        Fact* factFltTime = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "STAT_FLTTIME");
        hobbsTimeSeconds = (uint64_t)factFltTime->rawValue().toUInt();
        qCDebug(VehicleLog) << "Hobbs Meter raw Ardupilot(s):" << "(" <<  hobbsTimeSeconds << ")";
    }

    int hours   = hobbsTimeSeconds / 3600;
    int minutes = (hobbsTimeSeconds % 3600) / 60;
    int seconds = hobbsTimeSeconds % 60;
    QString timeStr = QString::asprintf("%04d:%02d:%02d", hours, minutes, seconds);
    qCDebug(VehicleLog) << "Hobbs Meter string:" << timeStr;
    return timeStr;
} 

bool APMFirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    return vehicle->flightMode() == "Guided";
}

void APMFirmwarePlugin::_soloVideoHandshake(void)
{
    QTcpSocket* socket = new QTcpSocket(this);

    QObject::connect(socket, &QAbstractSocket::errorOccurred, this, &APMFirmwarePlugin::_artooSocketError);
    socket->connectToHost(_artooIP, _artooVideoHandshakePort);
}

void APMFirmwarePlugin::_artooSocketError(QAbstractSocket::SocketError socketError)
{
    qgcApp()->showAppMessage(tr("Error during Solo video link setup: %1").arg(socketError));
}

QString APMFirmwarePlugin::_internalParameterMetaDataFile(Vehicle* vehicle)
{
    switch (vehicle->vehicleType()) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
        if (vehicle->versionCompare(4, 1, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.1.xml");
        }
        if (vehicle->versionCompare(4, 0, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.0.xml");
        }
        if (vehicle->versionCompare(3, 7, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Copter.3.7.xml");
        }
        if (vehicle->versionCompare(3, 6, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Copter.3.6.xml");
        }
        return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Copter.3.5.xml");

    case MAV_TYPE_VTOL_DUOROTOR:
    case MAV_TYPE_VTOL_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_RESERVED2:
    case MAV_TYPE_VTOL_RESERVED3:
    case MAV_TYPE_VTOL_RESERVED4:
    case MAV_TYPE_VTOL_RESERVED5:
    case MAV_TYPE_FIXED_WING:
        if (vehicle->versionCompare(4, 1, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.1.xml");
        }
        if (vehicle->versionCompare(4, 0, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Plane.4.0.xml");
        }
        if (vehicle->versionCompare(3, 10, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Plane.3.10.xml");
        }
        if (vehicle->versionCompare(3, 9, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Plane.3.9.xml");
        }
        return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Plane.3.8.xml");

    case MAV_TYPE_GROUND_ROVER:
    case MAV_TYPE_SURFACE_BOAT:
        if (vehicle->versionCompare(4, 1, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.1.xml");
        }
        if (vehicle->versionCompare(4, 0, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Rover.4.0.xml");
        }
        if (vehicle->versionCompare(3, 6, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Rover.3.6.xml");
        }
        if (vehicle->versionCompare(3, 5, 0) >= 0) {
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Rover.3.5.xml");
        }
        return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Rover.3.4.xml");

    case MAV_TYPE_SUBMARINE:
        if (vehicle->versionCompare(4, 1, 0) >= 0) { // 4.1.x
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Sub.4.1.xml");
        }
        if (vehicle->versionCompare(4, 0, 0) >= 0) { // 4.0.x
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Sub.4.0.xml");
        }
        if (vehicle->versionCompare(3, 6, 0) >= 0) { // 3.6.x
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Sub.3.6.xml");
        }
        if (vehicle->versionCompare(3, 5, 0) >= 0) { // 3.5.x
            return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Sub.3.5.xml");
        }
        // up to 3.4.x
        return QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Sub.3.4.xml");

    default:
        return QString();
    }
}

void APMFirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    if (guidedMode) {
        _setFlightModeAndValidate(vehicle, "Guided");
    } else {
        pauseVehicle(vehicle);
    }
}

void APMFirmwarePlugin::pauseVehicle(Vehicle* vehicle)
{
    _setFlightModeAndValidate(vehicle, pauseFlightMode());
}

void APMFirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showAppMessage(QStringLiteral("Unable to go to location, vehicle position not known."));
        return;
    }


    setGuidedMode(vehicle, true);

    QGeoCoordinate coordWithAltitude = gotoCoord;
    coordWithAltitude.setAltitude(vehicle->altitudeRelative()->rawValue().toDouble());
    vehicle->missionManager()->writeArduPilotGuidedMissionItem(coordWithAltitude, false /* altChangeOnly */);
}

void APMFirmwarePlugin::guidedModeRTL(Vehicle* vehicle, bool smartRTL)
{
    _setFlightModeAndValidate(vehicle, smartRTL ? smartRTLFlightMode() : rtlFlightMode());
}

void APMFirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange, bool pauseVehicle)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showAppMessage(tr("Unable to change altitude, vehicle altitude not known."));
        return;
    }

    if (pauseVehicle && !_setFlightModeAndValidate(vehicle, pauseFlightMode())) {
        qgcApp()->showAppMessage(tr("Unable to pause vehicle."));
        return;
    }

    if (abs(altitudeChange) < 0.01) {
        // This prevents unecessary changes to Guided mode when the users selects pause and doesn't really touch the altitude slider
        return;
    }

    setGuidedMode(vehicle, true);

    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t                       msg;
        mavlink_set_position_target_local_ned_t cmd;

        memset(&cmd, 0, sizeof(cmd));

        cmd.target_system    = static_cast<uint8_t>(vehicle->id());
        cmd.target_component = static_cast<uint8_t>(vehicle->defaultComponentId());
        cmd.coordinate_frame = MAV_FRAME_LOCAL_OFFSET_NED;
        cmd.type_mask = 0xFFF8; // Only x/y/z valid
        cmd.x = 0.0f;
        cmd.y = 0.0f;
        cmd.z = static_cast<float>(-(altitudeChange));

        MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_msg_set_position_target_local_ned_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &cmd);

        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void APMFirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double altitudeRel)
{
    _guidedModeTakeoff(vehicle, altitudeRel);
}

double APMFirmwarePlugin::minimumTakeoffAltitude(Vehicle* vehicle)
{
    double minTakeoffAlt = 0;
    QString takeoffAltParam(vehicle->vtol() ? QStringLiteral("Q_RTL_ALT") : QStringLiteral("PILOT_TKOFF_ALT"));
    float paramDivisor = vehicle->vtol() ? 1.0 : 100.0; // PILOT_TAKEOFF_ALT is in centimeters

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, takeoffAltParam)) {
        minTakeoffAlt = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, takeoffAltParam)->rawValue().toDouble() / static_cast<double>(paramDivisor);
    }

    if (minTakeoffAlt == 0) {
        minTakeoffAlt = FirmwarePlugin::minimumTakeoffAltitude(vehicle);
    }

    return minTakeoffAlt;
}

bool APMFirmwarePlugin::_guidedModeTakeoff(Vehicle* vehicle, double altitudeRel)
{
    if (!vehicle->multiRotor() && !vehicle->vtol()) {
        qgcApp()->showAppMessage(tr("Vehicle does not support guided takeoff"));
        return false;
    }

    double vehicleAltitudeAMSL = vehicle->altitudeAMSL()->rawValue().toDouble();
    if (qIsNaN(vehicleAltitudeAMSL)) {
        qgcApp()->showAppMessage(tr("Unable to takeoff, vehicle position not known."));
        return false;
    }

    double takeoffAltRel = minimumTakeoffAltitude(vehicle);
    if (!qIsNaN(altitudeRel) && altitudeRel > takeoffAltRel) {
        takeoffAltRel = altitudeRel;
    }

    if (!_setFlightModeAndValidate(vehicle, "Guided")) {
        qgcApp()->showAppMessage(tr("Unable to takeoff: Vehicle failed to change to Guided mode."));
        return false;
    }

    if (!_armVehicleAndValidate(vehicle)) {
        qgcApp()->showAppMessage(tr("Unable to takeoff: Vehicle failed to arm."));
        return false;
    }

    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_NAV_TAKEOFF,
                            true, // show error
                            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            static_cast<float>(takeoffAltRel));                     // Relative altitude

    return true;
}

void APMFirmwarePlugin::startMission(Vehicle* vehicle)
{
    if (vehicle->flying()) {
        // Vehicle already in the air, we just need to switch to auto
        if (!_setFlightModeAndValidate(vehicle, "Auto")) {
            qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle failed to change to Auto mode."));
        }
        return;
    }

    if (!vehicle->armed()) {
        // First switch to flight mode we can arm from
        if (!_setFlightModeAndValidate(vehicle, "Guided")) {
            qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle failed to change to Guided mode."));
            return;
        }

        if (!_armVehicleAndValidate(vehicle)) {
            qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle failed to arm."));
            return;
        }
    }

    if (vehicle->fixedWing()) {
        if (!_setFlightModeAndValidate(vehicle, "Auto")) {
            qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle failed to change to Auto mode."));
            return;
        }
    } else {
        vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_MISSION_START, true /*show error */);
    }
}

QString APMFirmwarePlugin::_getLatestVersionFileUrl(Vehicle* vehicle)
{
    const static QString baseUrl("http://firmware.ardupilot.org/%1/stable/Pixhawk1/git-version.txt");

    if (qobject_cast<ArduPlaneFirmwarePlugin*>(vehicle->firmwarePlugin())) {
        return baseUrl.arg("Plane");
    } else if (qobject_cast<ArduRoverFirmwarePlugin*>(vehicle->firmwarePlugin())) {
        return baseUrl.arg("Rover");
    } else if (qobject_cast<ArduSubFirmwarePlugin*>(vehicle->firmwarePlugin())) {
        return baseUrl.arg("Sub");
    } else if (qobject_cast<ArduCopterFirmwarePlugin*>(vehicle->firmwarePlugin())) {
        return baseUrl.arg("Copter");
    } else {
        qWarning() << "APMFirmwarePlugin::_getLatestVersionFileUrl Unknown vehicle firmware type" << vehicle->vehicleType();
        return QString();
    }
}

QString APMFirmwarePlugin::_versionRegex() {
    return QStringLiteral(" V([0-9,\\.]*)$");
}

void APMFirmwarePlugin::_handleRCChannels(Vehicle* vehicle, mavlink_message_t* message)
{
    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_rc_channels_t   channels;

        mavlink_msg_rc_channels_decode(message, &channels);
        //-- Ardupilot uses 0-255 to indicate 0-100% where QGC expects 0-100
        if(channels.rssi) {
            channels.rssi = static_cast<uint8_t>(static_cast<double>(channels.rssi) / 255.0 * 100.0);
        }
        MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_msg_rc_channels_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    message,
                    &channels);
    }
}

void APMFirmwarePlugin::_handleRCChannelsRaw(Vehicle* vehicle, mavlink_message_t *message)
{
    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_rc_channels_raw_t   channels;

        mavlink_msg_rc_channels_raw_decode(message, &channels);
        //-- Ardupilot uses 0-255 to indicate 0-100% where QGC expects 0-100
        if(channels.rssi) {
            channels.rssi = static_cast<uint8_t>(static_cast<double>(channels.rssi) / 255.0 * 100.0);
        }
        MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_msg_rc_channels_raw_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    message,
                    &channels);
    }
}

void APMFirmwarePlugin::_sendGCSMotionReport(Vehicle* vehicle, FollowMe::GCSMotionReport& motionReport, uint8_t estimationCapabilities)
{
    if (!vehicle->homePosition().isValid()) {
        static bool sentOnce = false;
        if (!sentOnce) {
            sentOnce = true;
            qgcApp()->showAppMessage(tr("Follow failed: Home position not set."));
        }
        return;
    }

    if (!(estimationCapabilities & (FollowMe::POS | FollowMe::VEL | FollowMe::HEADING))) {
        static bool sentOnce = false;
        if (!sentOnce) {
            sentOnce = true;
            qWarning() << "APMFirmwarePlugin::_sendGCSMotionReport estimateCapabilities" << estimationCapabilities;
            qgcApp()->showAppMessage(tr("Follow failed: Ground station cannot provide required position information."));
        }
        return;
    }

    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        MAVLinkProtocol*                mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_global_position_int_t   globalPositionInt;

        memset(&globalPositionInt, 0, sizeof(globalPositionInt));

        // Important note: QGC only supports sending the constant GCS home position altitude for follow me.
        globalPositionInt.time_boot_ms =    static_cast<uint32_t>(qgcApp()->msecsSinceBoot());
        globalPositionInt.lat =             motionReport.lat_int;
        globalPositionInt.lon =             motionReport.lon_int;
        globalPositionInt.alt =             static_cast<int32_t>(vehicle->homePosition().altitude() * 1000);    // mm
        globalPositionInt.relative_alt =    static_cast<int32_t>(0);                                            // mm
        globalPositionInt.vx =              static_cast<int16_t>(motionReport.vxMetersPerSec * 100);            // cm/sec
        globalPositionInt.vy =              static_cast<int16_t>(motionReport.vyMetersPerSec * 100);            // cm/sec
        globalPositionInt.vy =              static_cast<int16_t>(motionReport.vzMetersPerSec * 100);            // cm/sec
        globalPositionInt.hdg =             static_cast<uint16_t>(motionReport.headingDegrees * 100.0);         // centi-degrees

        mavlink_message_t message;
        mavlink_msg_global_position_int_encode_chan(static_cast<uint8_t>(mavlinkProtocol->getSystemId()),
                                                    static_cast<uint8_t>(mavlinkProtocol->getComponentId()),
                                                    sharedLink->mavlinkChannel(),
                                                    &message,
                                                    &globalPositionInt);
        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }
}

uint8_t APMFirmwarePlugin::_reencodeMavlinkChannel()
{
    // This mutex is only to guard against a race on allocating the channel id
    // if two firmware plugins are created simultaneously from different threads
    //
    // Use of the allocated channel should be guarded by the mutex returned from
    // _reencodeMavlinkChannelMutex()
    //
    static QMutex _channelMutex{};
    _channelMutex.lock();
    static uint8_t channel{LinkManager::invalidMavlinkChannel()};
    if (LinkManager::invalidMavlinkChannel() == channel) {
        channel = qgcApp()->toolbox()->linkManager()->allocateMavlinkChannel();
    }
    _channelMutex.unlock();
    return channel;
}

QMutex& APMFirmwarePlugin::_reencodeMavlinkChannelMutex()
{
    static QMutex _mutex{};
    return _mutex;
}
