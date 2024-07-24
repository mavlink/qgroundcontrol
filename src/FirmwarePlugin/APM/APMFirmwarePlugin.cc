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
#include "SettingsManager.h"
#include "AppSettings.h"
#include "APMMavlinkStreamRateSettings.h"
#include "ArduPlaneFirmwarePlugin.h"
#include "ArduCopterFirmwarePlugin.h"
#include "ArduRoverFirmwarePlugin.h"
#include "ArduSubFirmwarePlugin.h"
#include "APMParameterMetaData.h"
#include "LinkManager.h"
#include "Vehicle.h"
#include <StatusTextHandler.h>
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include <DeviceInfo.h>

#include <QtNetwork/QTcpSocket>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>

QGC_LOGGING_CATEGORY(APMFirmwarePluginLog, "APMFirmwarePluginLog")

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
    uint32_t available = SetFlightModeCapability | PauseVehicleCapability | GuidedModeCapability | ROIModeCapability;
    if (vehicle->multiRotor()) {
        available |= TakeoffVehicleCapability;
        available |= ROIModeCapability;
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

    const QString messageText = StatusTextHandler::getMessageText(*message);
    if (messageText.contains("Place vehicle") || messageText.contains("Calibration successful")) {
        _adjustCalibrationMessageSeverity(message);
        return true;
    }

    static const QRegularExpression APM_FRAME_REXP("^Frame: (\\S*)");
    const QRegularExpressionMatch match = APM_FRAME_REXP.match(messageText);
    if (match.hasMatch()) {
        static const QSet<QString> coaxialFrames = {"Y6", "OCTA_QUAD", "COAX", "QUAD/PLUS"};
        const QString frameType = match.captured(1);
        _coaxialMotors = coaxialFrames.contains(frameType);
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
        case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
        case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_FIXEDROTOR:
        case MAV_TYPE_VTOL_TAILSITTER:
        case MAV_TYPE_VTOL_TILTWING:
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

void APMFirmwarePlugin::_getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
{
    APMParameterMetaData::getParameterMetaDataVersionInfo(metaDataFile, majorVersion, minorVersion);
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
        MAV_CMD_DO_SET_ROI_LOCATION, MAV_CMD_DO_SET_ROI_NONE,
        MAV_CMD_DO_DIGICAM_CONFIGURE, MAV_CMD_DO_DIGICAM_CONTROL,
        MAV_CMD_DO_MOUNT_CONTROL,
        MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
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

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "STAT_FLTTIME")) {
        Fact* factFltTime = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "STAT_FLTTIME");
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

bool APMFirmwarePlugin::hasGripper(const Vehicle* vehicle) const
{
    if(vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "GRIP_ENABLE")) {
        bool _hasGripper = (vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("GRIP_ENABLE"))->rawValue().toInt()) == 1 ? true : false;
        return _hasGripper;
    }
    return false;
}

const QVariantList& APMFirmwarePlugin::toolIndicators(const Vehicle* vehicle)
{
    if (_toolIndicatorList.size() == 0) {
        // First call the base class to get the standard QGC list
        _toolIndicatorList = FirmwarePlugin::toolIndicators(vehicle);

        // Find the generic flight mode indicator and replace with the custom one
        for (int i=0; i<_toolIndicatorList.size(); i++) {
            if (_toolIndicatorList.at(i).toUrl().toString().contains("FlightModeIndicator.qml")) {
                _toolIndicatorList[i] = QVariant::fromValue(QUrl::fromUserInput("qrc:/APM/Indicators/APMFlightModeIndicator.qml"));
                break;
            }
        }

        // Find the generic battery indicator and replace with the custom one
        for (int i=0; i<_toolIndicatorList.size(); i++) {
            if (_toolIndicatorList.at(i).toUrl().toString().contains("BatteryIndicator.qml")) {
                _toolIndicatorList[i] = QVariant::fromValue(QUrl::fromUserInput("qrc:/APM/Indicators/APMBatteryIndicator.qml"));
                break;
            }
        }

        // Then add the forwarding support indicator
        _toolIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/APMSupportForwardingIndicator.qml")));
    }

    return _toolIndicatorList;
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

QString APMFirmwarePlugin::_vehicleClassToString(QGCMAVLink::VehicleClass_t vehicleClass) const
{
    switch (vehicleClass) {
    case QGCMAVLink::VehicleClassMultiRotor:
        return QStringLiteral("Copter");

    case QGCMAVLink::VehicleClassFixedWing:
    case QGCMAVLink::VehicleClassVTOL:
        return QStringLiteral("Plane");

    case QGCMAVLink::VehicleClassRoverBoat:
        return QStringLiteral("Rover");

    case QGCMAVLink::VehicleClassSub:
        return QStringLiteral("Sub");

    default:
        qCWarning(APMFirmwarePluginLog) << Q_FUNC_INFO << "called with bad VehicleClass_t:" << vehicleClass;
        return QString();
    }
}

QString APMFirmwarePlugin::_internalParameterMetaDataFile(const Vehicle* vehicle) const
{
    const QGCMAVLink::VehicleClass_t vehicleClass = QGCMAVLink::vehicleClass(vehicle->vehicleType());

    const QString vehicleName = _vehicleClassToString(vehicleClass);
    if(vehicleName.isEmpty()) {
        qCWarning(APMFirmwarePluginLog) << Q_FUNC_INFO << "called with bad VehicleClass_t:" << vehicleClass;
        return QString();
    }

    const QString fileNameFormat = QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.%1.%2.%3.xml");
    int currMajor = vehicle->firmwareMajorVersion();
    int currMinor = vehicle->firmwareMinorVersion();

    // Find next newest version available
    while ((currMajor >= 3) && (currMinor > 0)) {
        const QString tempFileName = fileNameFormat.arg(vehicleName).arg(currMajor).arg(currMinor);
        if (QFileInfo::exists(tempFileName)) {
            return tempFileName;
        }
        currMinor--;
        if (currMinor == 0) {
            currMinor = 10; // Some suitably high possible minor version.
            currMajor--;
        }
    }
    // currMajor or currMinor were likely invalid (-1)

    // Use oldest version available which should be equivalent to offline params
    for (int i = 0; i < 10; i++) {
        const QString tempFileName = fileNameFormat.arg(vehicleName).arg(3).arg(i);
        if (QFileInfo::exists(tempFileName)) {
            return tempFileName;
        }
    }

    qCWarning(APMFirmwarePluginLog) << Q_FUNC_INFO << "Meta Data File Not Found";
    return QString();
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

typedef struct {
    Vehicle*            vehicle;
} MAV_CMD_DO_REPOSITION_HandlerData_t;

static void _MAV_CMD_DO_REPOSITION_ResultHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t /*failureCode*/)
{
    auto* data = (MAV_CMD_DO_REPOSITION_HandlerData_t*)resultHandlerData;
    auto* vehicle = data->vehicle;
    auto* instanceData = qobject_cast<APMFirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());

    if (instanceData->MAV_CMD_DO_REPOSITION_supported ||
        instanceData->MAV_CMD_DO_REPOSITION_unsupported) {
        // we never change out minds once set
        goto out;
    }

    instanceData->MAV_CMD_DO_REPOSITION_supported = (ack.result == MAV_RESULT_ACCEPTED);
    instanceData->MAV_CMD_DO_REPOSITION_unsupported = (ack.result == MAV_RESULT_UNSUPPORTED);

out:
    delete data;
}

void APMFirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showAppMessage(QStringLiteral("Unable to go to location, vehicle position not known."));
        return;
    }

    // attempt to use MAV_CMD_DO_REPOSITION to move vehicle.  If that
    // comes back as unsupported, try using the old system of sending
    // through mission items with custom "current" field values.
    auto* instanceData = qobject_cast<APMFirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());

    // if we know it is supported or we don't know for sure it is
    // unsupported then send the command:
    if (instanceData) {
        if (instanceData->MAV_CMD_DO_REPOSITION_supported ||
            !instanceData->MAV_CMD_DO_REPOSITION_unsupported) {
            auto* result_handler_data = new MAV_CMD_DO_REPOSITION_HandlerData_t{
                vehicle
            };

            Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
            handlerInfo.resultHandler       = _MAV_CMD_DO_REPOSITION_ResultHandler;
            handlerInfo.resultHandlerData   = result_handler_data;

            vehicle->sendMavCommandIntWithHandler(
                &handlerInfo,
                vehicle->defaultComponentId(),
                MAV_CMD_DO_REPOSITION,
                MAV_FRAME_GLOBAL,
                -1.0f,
                MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                0.0f,
                NAN,
                gotoCoord.latitude(),
                gotoCoord.longitude(),
                vehicle->altitudeAMSL()->rawValue().toFloat()
                );
        }
        if (instanceData->MAV_CMD_DO_REPOSITION_supported) {
            // no need to fall back
            return;
        }
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

bool APMFirmwarePlugin::mulirotorSpeedLimitsAvailable(Vehicle* vehicle)
{
    return vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "WPNAV_SPEED");
}

double APMFirmwarePlugin::maximumHorizontalSpeedMultirotor(Vehicle* vehicle)
{
    QString speedParam("WPNAV_SPEED");

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, speedParam)) {
        return vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, speedParam)->rawValue().toDouble() * 0.01;  // note cm/s -> m/s
    }

    return FirmwarePlugin::maximumHorizontalSpeedMultirotor(vehicle);
}

void APMFirmwarePlugin::guidedModeChangeGroundSpeedMetersSecond(Vehicle *vehicle, double groundspeed)
{
    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_DO_CHANGE_SPEED,
        true,                                   // show error is fails
        1,                                     // 0: airspeed, 1: groundspeed
        static_cast<float>(groundspeed),       // groundspeed setpoint
        -1,                                   // throttle
        0,                                    // 0: absolute speed, 1: relative to current
        NAN, NAN,NAN);                        // param 5-7 unused
}

void APMFirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double altitudeRel)
{
    _guidedModeTakeoff(vehicle, altitudeRel);
}

double APMFirmwarePlugin::minimumTakeoffAltitudeMeters(Vehicle* vehicle)
{
    double minTakeoffAlt = 0;
    QString takeoffAltParam(vehicle->vtol() ? QStringLiteral("Q_RTL_ALT") : QStringLiteral("PILOT_TKOFF_ALT"));
    float paramDivisor = vehicle->vtol() ? 1.0 : 100.0; // PILOT_TAKEOFF_ALT is in centimeters

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, takeoffAltParam)) {
        minTakeoffAlt = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, takeoffAltParam)->rawValue().toDouble() / static_cast<double>(paramDivisor);
    }

    if (minTakeoffAlt == 0) {
        minTakeoffAlt = FirmwarePlugin::minimumTakeoffAltitudeMeters(vehicle);
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

    double takeoffAltRel = minimumTakeoffAltitudeMeters(vehicle);
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
        // In Ardupilot for vtols and airplanes we need to set the mode to auto and then arm, otherwise if arming in guided
        // If the vehicle has tilt rotors, it will arm them in forward flight position, being dangerous.
        if (vehicle->fixedWing()) {
            if (!_setFlightModeAndValidate(vehicle, "Auto")) {
                qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle failed to change to Auto mode."));
                return;
            }
        } else {
            if (!_setFlightModeAndValidate(vehicle, "Guided")) {
                qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle failed to change to Guided mode."));
                return;
            }
        }

        if (!_armVehicleAndValidate(vehicle)) {
            qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle failed to arm."));
            return;
        }
    }

    // For non aircraft vehicles, we would be in guided mode, so we need to send the mission start command
    if (!vehicle->fixedWing()) {
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
        //-- Ardupilot uses 0-254 to indicate 0-100% where QGC expects 0-100
        // As per mavlink specs, 255 means invalid, we must leave it like that for indicators to hide if no rssi data
        if(channels.rssi && channels.rssi != 255) {
            channels.rssi = static_cast<uint8_t>(static_cast<double>(channels.rssi) / 254.0 * 100.0);
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

double APMFirmwarePlugin::maximumEquivalentAirspeed(Vehicle* vehicle)
{
    QString airspeedMax("ARSPD_FBW_MAX");

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, airspeedMax)) {
        return vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, airspeedMax)->rawValue().toDouble();
    }

    return FirmwarePlugin::maximumEquivalentAirspeed(vehicle);
}

double APMFirmwarePlugin::minimumEquivalentAirspeed(Vehicle* vehicle)
{
    QString airspeedMin("ARSPD_FBW_MIN");

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, airspeedMin)) {
        return vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, airspeedMin)->rawValue().toDouble();
    }

    return FirmwarePlugin::minimumEquivalentAirspeed(vehicle);
}

bool APMFirmwarePlugin::fixedWingAirSpeedLimitsAvailable(Vehicle* vehicle)
{
    return vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "ARSPD_FBW_MIN") &&
           vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "ARSPD_FBW_MAX");
}

void APMFirmwarePlugin::guidedModeChangeEquivalentAirspeedMetersSecond(Vehicle* vehicle, double airspeed_equiv)
{

    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_DO_CHANGE_SPEED,
        true,                                 // show error is fails
        0,                                    // 0: airspeed, 1: groundspeed
        static_cast<float>(airspeed_equiv),   // speed setpoint
        -1,                                   // throttle, no change
        0                                     // 0: absolute speed, 1: relative to current
        );                                    // param 5-7 unused
}

QVariant APMFirmwarePlugin::mainStatusIndicatorContentItem(const Vehicle*) const
{
    return QVariant::fromValue(QUrl::fromUserInput("qrc:/APM/Indicators/APMMainStatusIndicatorContentItem.qml"));
}

void APMFirmwarePlugin::_setBaroGndTemp(Vehicle* vehicle, qreal temp)
{
    if (!vehicle) {
        return;
    }

    const QString bareGndTemp("BARO_GND_TEMP");

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, bareGndTemp)) {
        Fact* const param = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, bareGndTemp);
        param->setRawValue(temp);
    }
}

void APMFirmwarePlugin::_setBaroAltOffset(Vehicle* vehicle, qreal offset)
{
    if (!vehicle) {
        return;
    }

    const QString baroAltOffset("BARO_ALT_OFFSET");

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, baroAltOffset)) {
        Fact* const param = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, baroAltOffset);
        param->setRawValue(offset);
    }
}

#define ALT_METERS                          (145366.45 * 0.3048)
#define ALT_EXP                             (1. / 5.225)
#define SEA_PRESSURE                        101325.
#define CELSIUS_TO_KELVIN(celsius)          (celsius + 273.15)
#define ALT_OFFSET_P(pressure)              (1. - pow((pressure / SEA_PRESSURE), ALT_EXP))
#define ALT_OFFSET_PT(pressure,temperature) ((ALT_OFFSET_P(pressure) * CELSIUS_TO_KELVIN(temperature)) / 0.0065)

qreal APMFirmwarePlugin::calcAltOffsetPT(uint32_t atmospheric1, qreal temperature1, uint32_t atmospheric2, qreal temperature2)
{
    const qreal alt1 = ALT_OFFSET_PT(atmospheric1, temperature1);
    const qreal alt2 = ALT_OFFSET_PT(atmospheric2, temperature2);
    const qreal offset = alt1 - alt2;
    return offset;
}

qreal APMFirmwarePlugin::calcAltOffsetP(uint32_t atmospheric1, uint32_t atmospheric2)
{
    const qreal alt1 = ALT_OFFSET_P(atmospheric1) * ALT_METERS;
    const qreal alt2 = ALT_OFFSET_P(atmospheric2) * ALT_METERS;
    const qreal offset = alt1 - alt2;
    return offset;
}

QPair<QMetaObject::Connection,QMetaObject::Connection> APMFirmwarePlugin::startCompensatingBaro(Vehicle* vehicle)
{
    // TODO: Running Average?
    const QMetaObject::Connection baroPressureUpdater = QObject::connect(QGCDeviceInfo::QGCPressure::instance(), &QGCDeviceInfo::QGCPressure::pressureUpdated, vehicle, [vehicle](qreal pressure, qreal temperature){
        if (!vehicle || !vehicle->flying()) {
            return;
        }

        if (qFuzzyIsNull(pressure)) {
            return;
        }

        const qreal initialPressure = vehicle->getInitialGCSPressure();
        if (qFuzzyIsNull(initialPressure)) {
            return;
        }

        const qreal initialTemperature = vehicle->getInitialGCSTemperature();

        qreal offset = 0.;
        if (!qFuzzyIsNull(temperature) && !qFuzzyIsNull(initialTemperature)) {
            offset = APMFirmwarePlugin::calcAltOffsetPT(initialPressure, initialTemperature, pressure, temperature);
        } else {
            offset = APMFirmwarePlugin::calcAltOffsetP(initialPressure, pressure);
        }

        APMFirmwarePlugin::_setBaroAltOffset(vehicle, offset);
    });

    const QMetaObject::Connection baroTempUpdater = connect(QGCDeviceInfo::QGCAmbientTemperature::instance(), &QGCDeviceInfo::QGCAmbientTemperature::temperatureUpdated, vehicle, [vehicle](qreal temperature){
        if (!vehicle || !vehicle->flying()) {
           return;
        }

        if (qFuzzyIsNull(temperature)) {
            return;
        }

        APMFirmwarePlugin::_setBaroGndTemp(vehicle, temperature);
    });

    return QPair<QMetaObject::Connection,QMetaObject::Connection>(baroPressureUpdater, baroTempUpdater);
}

bool APMFirmwarePlugin::stopCompensatingBaro(const Vehicle* vehicle, QPair<QMetaObject::Connection,QMetaObject::Connection> updaters)
{
    Q_UNUSED(vehicle);
    /*if (!vehicle) {
        return false;
    }*/

    bool result = false;

    if (updaters.first) {
        result |= QObject::disconnect(updaters.first);
    }

    if (updaters.second) {
        result |= QObject::disconnect(updaters.second);
    }

    return result;
}
