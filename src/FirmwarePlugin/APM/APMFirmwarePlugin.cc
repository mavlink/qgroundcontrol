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

#include <QTcpSocket>

QGC_LOGGING_CATEGORY(APMFirmwarePluginLog, "APMFirmwarePluginLog")

static const QRegExp APM_COPTER_REXP("^(ArduCopter|APM:Copter)");
static const QRegExp APM_SOLO_REXP("^(APM:Copter solo-)");
static const QRegExp APM_PLANE_REXP("^(ArduPlane|APM:Plane)");
static const QRegExp APM_ROVER_REXP("^(ArduRover|APM:Rover)");
static const QRegExp APM_SUB_REXP("^(ArduSub|APM:Sub)");
static const QRegExp APM_PX4NUTTX_REXP("^PX4: .*NuttX: .*");
static const QRegExp APM_FRAME_REXP("^Frame: ");
static const QRegExp APM_SYSID_REXP("^PX4v2 ");

// Regex to parse version text coming from APM, gives out firmware type, major, minor and patch level numbers
static const QRegExp VERSION_REXP("^(APM:Copter|APM:Plane|APM:Rover|APM:Sub|ArduCopter|ArduPlane|ArduRover|ArduSub) +[vV](\\d*)\\.*(\\d*)*\\.*(\\d*)*");

// minimum firmware versions that don't suffer from mavlink severity inversion bug.
// https://github.com/diydrones/apm_planner/issues/788
static const QString MIN_SOLO_VERSION_WITH_CORRECT_SEVERITY_MSGS("APM:Copter solo-1.2.0");
static const QString MIN_COPTER_VERSION_WITH_CORRECT_SEVERITY_MSGS("APM:Copter V3.4.0");
static const QString MIN_PLANE_VERSION_WITH_CORRECT_SEVERITY_MSGS("APM:Plane V3.4.0");
static const QString MIN_SUB_VERSION_WITH_CORRECT_SEVERITY_MSGS("APM:Sub V3.4.0");
static const QString MIN_ROVER_VERSION_WITH_CORRECT_SEVERITY_MSGS("APM:Rover V2.6.0");

const char* APMFirmwarePlugin::_artooIP =                   "10.1.1.1"; ///< IP address of ARTOO controller
const int   APMFirmwarePlugin::_artooVideoHandshakePort =   5502;       ///< Port for video handshake on ARTOO controller

/*
 * @brief APMFirmwareVersion is a small class to represent the firmware version
 * It encabsules vehicleType, major version, minor version and patch level version
 * and provides accessors for the same.
 * isValid() can be used, to know whether version infromation is available or not
 * supports < operator
 */
APMFirmwareVersion::APMFirmwareVersion(const QString &versionText)
{
    _major         = 0;
    _minor         = 0;
    _patch         = 0;

    _parseVersion(versionText);
}

bool APMFirmwareVersion::isValid() const
{
    return !_versionString.isEmpty();
}

bool APMFirmwareVersion::isBeta() const
{
    return _versionString.contains(QStringLiteral(".rc"));
}

bool APMFirmwareVersion::isDev() const
{
    return _versionString.contains(QStringLiteral(".dev"));
}

bool APMFirmwareVersion::operator <(const APMFirmwareVersion& other) const
{
    int myVersion = _major << 16 | _minor << 8 | _patch ;
    int otherVersion = other.majorNumber() << 16 | other.minorNumber() << 8 | other.patchNumber();
    return myVersion < otherVersion;
}

void APMFirmwareVersion::_parseVersion(const QString &versionText)
{
    if (versionText.isEmpty()) {
        return;
    }


    if (VERSION_REXP.indexIn(versionText) == -1) {
        qCWarning(APMFirmwarePluginLog) << "firmware version regex didn't match anything"
                                        << "version text to be parsed" << versionText;
        return;
    }

    QStringList capturedTexts = VERSION_REXP.capturedTexts();

    if (capturedTexts.count() < 5) {
        qCWarning(APMFirmwarePluginLog) << "something wrong with parsing the version text, not hitting anything"
                                        << VERSION_REXP.captureCount() << VERSION_REXP.capturedTexts();
        return;
    }

    // successful extraction of version numbers
    // even though we could have collected the version string atleast
    // but if the parsing has faild, not much point
    _versionString = versionText;
    _vehicleType   = capturedTexts[1];
    _major         = capturedTexts[2].toInt();
    _minor         = capturedTexts[3].toInt();
    _patch         = capturedTexts[4].toInt();
}


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

APMFirmwarePluginInstanceData::APMFirmwarePluginInstanceData(QObject* parent)
    : QObject(parent)
    , textSeverityAdjustmentNeeded(false)
{

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
    mavlink_status_t* mavlinkStatusReEncode = mavlink_get_channel_status(0);
    mavlinkStatusReEncode->flags |= MAVLINK_STATUS_FLAG_IN_MAVLINK1;
    mavlink_msg_param_value_encode_chan(message->sysid,
                                        message->compid,
                                        0,                  // Re-encoding uses reserved channel 0
                                        message,
                                        &paramValue);
}

void APMFirmwarePlugin::_handleOutgoingParamSet(Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);

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

    mavlink_msg_param_set_encode_chan(message->sysid, message->compid, outgoingLink->mavlinkChannel(), message, &paramSet);
}

bool APMFirmwarePlugin::_handleIncomingStatusText(Vehicle* vehicle, mavlink_message_t* message, bool longVersion)
{
    QString messageText;
    APMFirmwarePluginInstanceData* instanceData = qobject_cast<APMFirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());

    int severity;
    if (longVersion) {
        severity = mavlink_msg_statustext_long_get_severity(message);
    } else {
        severity = mavlink_msg_statustext_get_severity(message);
    }

    if (vehicle->firmwareMajorVersion() == Vehicle::versionNotSetValue || severity < MAV_SEVERITY_NOTICE) {
        messageText = _getMessageText(message, longVersion);
        qCDebug(APMFirmwarePluginLog) << messageText;

        if (!messageText.contains(APM_SOLO_REXP)) {
            // if don't know firmwareVersion yet, try and see if this message contains it
            if (messageText.contains(APM_COPTER_REXP) || messageText.contains(APM_PLANE_REXP) || messageText.contains(APM_ROVER_REXP) || messageText.contains(APM_SUB_REXP)) {
                // found version string
                APMFirmwareVersion firmwareVersion(messageText);
                instanceData->textSeverityAdjustmentNeeded = _isTextSeverityAdjustmentNeeded(firmwareVersion);

                vehicle->setFirmwareVersion(firmwareVersion.majorNumber(), firmwareVersion.minorNumber(), firmwareVersion.patchNumber());

                int supportedMajorNumber = -1;
                int supportedMinorNumber = -1;

                switch (vehicle->vehicleType()) {
                case MAV_TYPE_VTOL_DUOROTOR:
                case MAV_TYPE_VTOL_QUADROTOR:
                case MAV_TYPE_VTOL_TILTROTOR:
                case MAV_TYPE_VTOL_RESERVED2:
                case MAV_TYPE_VTOL_RESERVED3:
                case MAV_TYPE_VTOL_RESERVED4:
                case MAV_TYPE_VTOL_RESERVED5:
                case MAV_TYPE_FIXED_WING:
                    supportedMajorNumber = 3;
                    supportedMinorNumber = 8;
                    break;
                case MAV_TYPE_QUADROTOR:
                    // Start TCP video handshake with ARTOO in case it's a Solo running ArduPilot firmware
                    _soloVideoHandshake(vehicle, false /* originalSoloFirmware */);
                case MAV_TYPE_COAXIAL:
                case MAV_TYPE_HELICOPTER:
                case MAV_TYPE_SUBMARINE:
                case MAV_TYPE_HEXAROTOR:
                case MAV_TYPE_OCTOROTOR:
                case MAV_TYPE_TRICOPTER:
                    supportedMajorNumber = 3;
                    supportedMinorNumber = 5;
                    break;
                case MAV_TYPE_GROUND_ROVER:
                case MAV_TYPE_SURFACE_BOAT:
                    supportedMajorNumber = 3;
                    supportedMinorNumber = 4;
                default:
                    break;
                }

                if (supportedMajorNumber != -1) {
                    if (firmwareVersion.majorNumber() < supportedMajorNumber ||
                            (firmwareVersion.majorNumber() == supportedMajorNumber && firmwareVersion.minorNumber() < supportedMinorNumber)) {
                        qgcApp()->showMessage(tr("QGroundControl fully supports Version %1.%2 and above. You are using a version prior to that. This combination is untested, you may run into unpredictable results.").arg(supportedMajorNumber).arg(supportedMinorNumber));
                    }
                }
            }
        }

        // APM user facing calibration messages come through as high severity, we need to parse them out
        // and lower the severity on them so that they don't pop in the users face.

        if (messageText.contains("Place vehicle") || messageText.contains("Calibration successful")) {
            _adjustCalibrationMessageSeverity(message);
            return true;
        }
    }

    // adjust mesasge if needed
    if (instanceData->textSeverityAdjustmentNeeded) {
        _adjustSeverity(message);
    }

    if (messageText.isEmpty()) {
        messageText = _getMessageText(message, longVersion);
    }

    // The following messages are incorrectly labeled as warning message.
    // Fixed in newer firmware (unreleased at this point), but still in older firmware.
    if (messageText.contains(APM_COPTER_REXP) || messageText.contains(APM_PLANE_REXP) || messageText.contains(APM_ROVER_REXP) || messageText.contains(APM_SUB_REXP) ||
            messageText.contains(APM_PX4NUTTX_REXP) || messageText.contains(APM_FRAME_REXP) || messageText.contains(APM_SYSID_REXP)) {
        _setInfoSeverity(message, longVersion);
    }

    if (messageText.contains(APM_SOLO_REXP)) {
        qDebug() << "Found Solo";
        vehicle->setSoloFirmware(true);

        // Fix up severity
        _setInfoSeverity(message, longVersion);

        // Start TCP video handshake with ARTOO
        _soloVideoHandshake(vehicle, true /* originalSoloFirmware */);
    } else if (messageText.contains(APM_FRAME_REXP)) {
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
    if (message->msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        // We need to look at all heartbeats that go by from any component
        _handleIncomingHeartbeat(vehicle, message);
        return true;
    }

    // Only translate messages which come from ArduPilot code. All other components are expected to follow current mavlink spec.
    if (_ardupilotComponentMap[vehicle->id()][message->compid]) {
        switch (message->msgid) {
        case MAVLINK_MSG_ID_PARAM_VALUE:
            _handleIncomingParamValue(vehicle, message);
            break;
        case MAVLINK_MSG_ID_STATUSTEXT:
            return _handleIncomingStatusText(vehicle, message, false /* longVersion */);
        case MAVLINK_MSG_ID_STATUSTEXT_LONG:
            return _handleIncomingStatusText(vehicle, message, true /* longVersion */);
        case MAVLINK_MSG_ID_RC_CHANNELS:
            _handleRCChannels(vehicle, message);
            break;
        case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
            _handleRCChannelsRaw(vehicle, message);
            break;
        }
    }

    return true;
}

void APMFirmwarePlugin::adjustOutgoingMavlinkMessage(Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message)
{
    switch (message->msgid) {
    case MAVLINK_MSG_ID_PARAM_SET:
        _handleOutgoingParamSet(vehicle, outgoingLink, message);
        break;
    }
}

QString APMFirmwarePlugin::_getMessageText(mavlink_message_t* message, bool longVersion) const
{
    QByteArray b;

    if (longVersion) {
        b.resize(MAVLINK_MSG_STATUSTEXT_LONG_FIELD_TEXT_LEN+1);
        mavlink_msg_statustext_long_get_text(message, b.data());
    } else {
        b.resize(MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN+1);
        mavlink_msg_statustext_get_text(message, b.data());
    }

    // Ensure NUL-termination
    b[b.length()-1] = '\0';
    return QString(b);
}

bool APMFirmwarePlugin::_isTextSeverityAdjustmentNeeded(const APMFirmwareVersion& firmwareVersion)
{
    if (!firmwareVersion.isValid()) {
        return false;
    }

    bool adjustmentNeeded = false;
    if (firmwareVersion.vehicleType().contains(APM_COPTER_REXP)) {
        if (firmwareVersion < APMFirmwareVersion(MIN_COPTER_VERSION_WITH_CORRECT_SEVERITY_MSGS)) {
            adjustmentNeeded = true;
        }
    } else if (firmwareVersion.vehicleType().contains(APM_PLANE_REXP)) {
        if (firmwareVersion < APMFirmwareVersion(MIN_PLANE_VERSION_WITH_CORRECT_SEVERITY_MSGS)) {
            adjustmentNeeded = true;
        }
    } else if (firmwareVersion.vehicleType().contains(APM_ROVER_REXP)) {
        if (firmwareVersion < APMFirmwareVersion(MIN_ROVER_VERSION_WITH_CORRECT_SEVERITY_MSGS)) {
            adjustmentNeeded = true;
        }
    } else if (firmwareVersion.vehicleType().contains(APM_SUB_REXP)) {
        if (firmwareVersion < APMFirmwareVersion(MIN_SUB_VERSION_WITH_CORRECT_SEVERITY_MSGS)) {
            adjustmentNeeded = true;
        }
    }

    return adjustmentNeeded;
}

void APMFirmwarePlugin::_adjustSeverity(mavlink_message_t* message) const
{
    // lets make QGC happy with right severity values
    mavlink_statustext_t statusText;
    mavlink_msg_statustext_decode(message, &statusText);
    switch(statusText.severity) {
    case MAV_SEVERITY_ALERT:    /* SEVERITY_LOW according to old codes */
        statusText.severity = MAV_SEVERITY_WARNING;
        break;
    case MAV_SEVERITY_CRITICAL: /*SEVERITY_MEDIUM according to old codes  */
        statusText.severity = MAV_SEVERITY_ALERT;
        break;
    case MAV_SEVERITY_ERROR:    /*SEVERITY_HIGH according to old codes */
        statusText.severity = MAV_SEVERITY_CRITICAL;
        break;
    }

    // Re-Encoding is always done using mavlink 1.0
    mavlink_status_t* mavlinkStatusReEncode = mavlink_get_channel_status(0);
    mavlinkStatusReEncode->flags |= MAVLINK_STATUS_FLAG_IN_MAVLINK1;
    mavlink_msg_statustext_encode_chan(message->sysid,
                                       message->compid,
                                       0,                  // Re-encoding uses reserved channel 0
                                       message,
                                       &statusText);
}

void APMFirmwarePlugin::_setInfoSeverity(mavlink_message_t* message, bool longVersion) const
{
    // Re-Encoding is always done using mavlink 1.0
    mavlink_status_t* mavlinkStatusReEncode = mavlink_get_channel_status(0);
    mavlinkStatusReEncode->flags |= MAVLINK_STATUS_FLAG_IN_MAVLINK1;

    if (longVersion) {
        mavlink_statustext_long_t statusTextLong;
        mavlink_msg_statustext_long_decode(message, &statusTextLong);

        statusTextLong.severity = MAV_SEVERITY_INFO;
        mavlink_msg_statustext_long_encode_chan(message->sysid,
                                                message->compid,
                                                0,                  // Re-encoding uses reserved channel 0
                                                message,
                                                &statusTextLong);
    } else {
        mavlink_statustext_t statusText;
        mavlink_msg_statustext_decode(message, &statusText);

        statusText.severity = MAV_SEVERITY_INFO;
        mavlink_msg_statustext_encode_chan(message->sysid,
                                           message->compid,
                                           0,                  // Re-encoding uses reserved channel 0
                                           message,
                                           &statusText);
    }
}

void APMFirmwarePlugin::_adjustCalibrationMessageSeverity(mavlink_message_t* message) const
{
    mavlink_statustext_t statusText;
    mavlink_msg_statustext_decode(message, &statusText);
    // Re-Encoding is always done using mavlink 1.0
    mavlink_status_t* mavlinkStatusReEncode = mavlink_get_channel_status(0);
    mavlinkStatusReEncode->flags |= MAVLINK_STATUS_FLAG_IN_MAVLINK1;
    statusText.severity = MAV_SEVERITY_INFO;
    mavlink_msg_statustext_encode_chan(message->sysid, message->compid, 0, message, &statusText);
}

void APMFirmwarePlugin::initializeStreamRates(Vehicle* vehicle)
{
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

    // ArduPilot only sends home position on first boot and then when it arms. It does not stream the position.
    // This means that depending on when QGC connects to the vehicle it may not have home position.
    // This can cause various features to not be available. So we request home position streaming ourselves.
    // The MAV_CMD_SET_MESSAGE_INTERVAL command is only supported on newer firmwares. So we set showError=false.
    // Which also means than on older firmwares you may be left with some missing features.
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL, false /* showError */, MAVLINK_MSG_ID_HOME_POSITION, 1000000 /* 1 second interval in usec */);
}


void APMFirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    vehicle->setFirmwarePluginInstanceData(new APMFirmwarePluginInstanceData);

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
        if (qgcApp()->toolbox()->settingsManager()->appSettings()->apmStartMavlinkStreams()->rawValue().toBool()) {
            // Streams are not started automatically on APM stack (sort of)
            initializeStreamRates(vehicle);
        }
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

void APMFirmwarePlugin::addMetaDataToFact(QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType)
{
    APMParameterMetaData* apmMetaData = qobject_cast<APMParameterMetaData*>(parameterMetaData);

    if (apmMetaData) {
        apmMetaData->addMetaDataToFact(fact, vehicleType);
    } else {
        qWarning() << "Internal error: pointer passed to APMFirmwarePlugin::addMetaDataToFact not APMParameterMetaData";
    }
}

QList<MAV_CMD> APMFirmwarePlugin::supportedMissionCommands(void)
{
    return {
        MAV_CMD_NAV_WAYPOINT,
        MAV_CMD_NAV_LOITER_UNLIM, MAV_CMD_NAV_LOITER_TURNS, MAV_CMD_NAV_LOITER_TIME,
        MAV_CMD_NAV_RETURN_TO_LAUNCH, MAV_CMD_NAV_LAND, MAV_CMD_NAV_TAKEOFF,
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
        MAV_CMD_NAV_VTOL_TAKEOFF, MAV_CMD_NAV_VTOL_LAND, MAV_CMD_DO_VTOL_TRANSITION,
#if 0
    // Waiting for module update
        MAV_CMD_DO_SET_REVERSE,
#endif
    };
}

QString APMFirmwarePlugin::missionCommandOverrides(MAV_TYPE vehicleType) const
{
    switch (vehicleType) {
    case MAV_TYPE_GENERIC:
        return QStringLiteral(":/json/APM/MavCmdInfoCommon.json");
    case MAV_TYPE_FIXED_WING:
        return QStringLiteral(":/json/APM/MavCmdInfoFixedWing.json");
    case MAV_TYPE_QUADROTOR:
        return QStringLiteral(":/json/APM/MavCmdInfoMultiRotor.json");
    case MAV_TYPE_VTOL_QUADROTOR:
        return QStringLiteral(":/json/APM/MavCmdInfoVTOL.json");
    case MAV_TYPE_SUBMARINE:
        return QStringLiteral(":/json/APM/MavCmdInfoSub.json");
    case MAV_TYPE_GROUND_ROVER:
        return QStringLiteral(":/json/APM/MavCmdInfoRover.json");
    default:
        qWarning() << "APMFirmwarePlugin::missionCommandOverrides called with bad MAV_TYPE:" << vehicleType;
        return QString();
    }
}

QObject* APMFirmwarePlugin::loadParameterMetaData(const QString& metaDataFile)
{
    Q_UNUSED(metaDataFile);

    APMParameterMetaData* metaData = new APMParameterMetaData();
    metaData->loadParameterFactMetaDataFile(metaDataFile);
    return metaData;
}

bool APMFirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    return vehicle->flightMode() == "Guided";
}

void APMFirmwarePlugin::_soloVideoHandshake(Vehicle* vehicle, bool originalSoloFirmware)
{
    Q_UNUSED(vehicle);

    QTcpSocket* socket = new QTcpSocket();

    socket->connectToHost(_artooIP, _artooVideoHandshakePort);
    if (originalSoloFirmware) {
        QObject::connect(socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &APMFirmwarePlugin::_artooSocketError);
    }
}

void APMFirmwarePlugin::_artooSocketError(QAbstractSocket::SocketError socketError)
{
    qgcApp()->showMessage(tr("Error during Solo video link setup: %1").arg(socketError));
}

QString APMFirmwarePlugin::internalParameterMetaDataFile(Vehicle* vehicle)
{
    switch (vehicle->vehicleType()) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
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
        qgcApp()->showMessage(QStringLiteral("Unable to go to location, vehicle position not known."));
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

void APMFirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showMessage(tr("Unable to change altitude, vehicle altitude not known."));
        return;
    }

    setGuidedMode(vehicle, true);

    mavlink_message_t msg;
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
                vehicle->priorityLink()->mavlinkChannel(),
                &msg,
                &cmd);

    vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
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
        qgcApp()->showMessage(tr("Vehicle does not support guided takeoff"));
        return false;
    }

    double vehicleAltitudeAMSL = vehicle->altitudeAMSL()->rawValue().toDouble();
    if (qIsNaN(vehicleAltitudeAMSL)) {
        qgcApp()->showMessage(tr("Unable to takeoff, vehicle position not known."));
        return false;
    }

    double takeoffAltRel = minimumTakeoffAltitude(vehicle);
    if (!qIsNaN(altitudeRel) && altitudeRel > takeoffAltRel) {
        takeoffAltRel = altitudeRel;
    }

    if (!_setFlightModeAndValidate(vehicle, "Guided")) {
        qgcApp()->showMessage(tr("Unable to takeoff: Vehicle failed to change to Guided mode."));
        return false;
    }

    if (!_armVehicleAndValidate(vehicle)) {
        qgcApp()->showMessage(tr("Unable to takeoff: Vehicle failed to arm."));
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
            qgcApp()->showMessage(tr("Unable to start mission: Vehicle failed to change to Auto mode."));
        }
        return;
    }

    if (!vehicle->armed()) {
        // First switch to flight mode we can arm from
        if (!_setFlightModeAndValidate(vehicle, "Guided")) {
            qgcApp()->showMessage(tr("Unable to start mission: Vehicle failed to change to Guided mode."));
            return;
        }

        if (!_armVehicleAndValidate(vehicle)) {
            qgcApp()->showMessage(tr("Unable to start mission: Vehicle failed to arm."));
            return;
        }
    }

    if (vehicle->fixedWing()) {
        if (!_setFlightModeAndValidate(vehicle, "Auto")) {
            qgcApp()->showMessage(tr("Unable to start mission: Vehicle failed to change to Auto mode."));
            return;
        }
    } else {
        vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_MISSION_START, true /*show error */);
    }
}

QString APMFirmwarePlugin::_getLatestVersionFileUrl(Vehicle* vehicle)
{
    const static QString baseUrl("http://firmware.ardupilot.org/%1/stable/PX4/git-version.txt");

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
    mavlink_rc_channels_t channels;
    mavlink_msg_rc_channels_decode(message, &channels);
    //-- Ardupilot uses 0-255 to indicate 0-100% where QGC expects 0-100
    if(channels.rssi) {
        channels.rssi = static_cast<uint8_t>(static_cast<double>(channels.rssi) / 255.0 * 100.0);
    }
    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_rc_channels_encode_chan(
                static_cast<uint8_t>(mavlink->getSystemId()),
                static_cast<uint8_t>(mavlink->getComponentId()),
                vehicle->priorityLink()->mavlinkChannel(),
                message,
                &channels);
}

void APMFirmwarePlugin::_handleRCChannelsRaw(Vehicle* vehicle, mavlink_message_t *message)
{
    mavlink_rc_channels_raw_t channels;
    mavlink_msg_rc_channels_raw_decode(message, &channels);
    //-- Ardupilot uses 0-255 to indicate 0-100% where QGC expects 0-100
    if(channels.rssi) {
        channels.rssi = static_cast<uint8_t>(static_cast<double>(channels.rssi) / 255.0 * 100.0);
    }
    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_rc_channels_raw_encode_chan(
                static_cast<uint8_t>(mavlink->getSystemId()),
                static_cast<uint8_t>(mavlink->getComponentId()),
                vehicle->priorityLink()->mavlinkChannel(),
                message,
                &channels);
}

void APMFirmwarePlugin::_sendGCSMotionReport(Vehicle* vehicle, FollowMe::GCSMotionReport& motionReport, uint8_t estimationCapabilities)
{
    if (!vehicle->homePosition().isValid()) {
        static bool sentOnce = false;
        if (!sentOnce) {
            sentOnce = true;
            qgcApp()->showMessage(tr("Follow failed: Home position not set."));
        }
        return;
    }

    if (!(estimationCapabilities & (FollowMe::POS | FollowMe::VEL | FollowMe::HEADING))) {
        static bool sentOnce = false;
        if (!sentOnce) {
            sentOnce = true;
            qWarning() << "APMFirmwarePlugin::_sendGCSMotionReport estimateCapabilities" << estimationCapabilities;
            qgcApp()->showMessage(tr("Follow failed: Ground station cannot provide required position information."));
        }
        return;
    }

    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();

    mavlink_global_position_int_t globalPositionInt;
    memset(&globalPositionInt, 0, sizeof(globalPositionInt));

    globalPositionInt.time_boot_ms =    static_cast<uint32_t>(qgcApp()->msecsSinceBoot());
    globalPositionInt.lat =             motionReport.lat_int;
    globalPositionInt.lon =             motionReport.lon_int;
    globalPositionInt.alt =             static_cast<int32_t>(motionReport.altMetersAMSL * 1000);                                        // mm
    globalPositionInt.relative_alt =    static_cast<int32_t>((motionReport.altMetersAMSL - vehicle->homePosition().altitude()) * 1000); // mm
    globalPositionInt.vx =              static_cast<int16_t>(motionReport.vxMetersPerSec * 100);                                        // cm/sec
    globalPositionInt.vy =              static_cast<int16_t>(motionReport.vyMetersPerSec * 100);                                        // cm/sec
    globalPositionInt.vy =              static_cast<int16_t>(motionReport.vzMetersPerSec * 100);                                        // cm/sec
    globalPositionInt.hdg =             static_cast<uint16_t>(motionReport.headingDegrees * 100.0);                                     // centi-degrees

    mavlink_message_t message;
    mavlink_msg_global_position_int_encode_chan(static_cast<uint8_t>(mavlinkProtocol->getSystemId()),
                                          static_cast<uint8_t>(mavlinkProtocol->getComponentId()),
                                          vehicle->priorityLink()->mavlinkChannel(),
                                          &message,
                                          &globalPositionInt);
    vehicle->sendMessageOnLink(vehicle->priorityLink(), message);
}
