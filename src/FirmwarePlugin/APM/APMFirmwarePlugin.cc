/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "APMFirmwarePlugin.h"
#include "Generic/GenericFirmwarePlugin.h"
#include "AutoPilotPlugins/APM/APMAutoPilotPlugin.h"    // FIXME: Hack
#include "QGCMAVLink.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(APMFirmwarePluginLog, "APMFirmwarePluginLog")

static const QRegExp APM_COPTER_REXP("^(ArduCopter|APM:Copter)");
static const QRegExp APM_PLANE_REXP("^(ArduPlane|APM:Plane)");
static const QRegExp APM_ROVER_REXP("^(ArduRover|APM:Rover)");
static const QRegExp APM_PX4NUTTX_REXP("^PX4: .*NuttX: .*");
static const QRegExp APM_FRAME_REXP("^Frame: ");
static const QRegExp APM_SYSID_REXP("^PX4v2 ");

// Regex to parse version text coming from APM, gives out firmware type, major, minor and patch level numbers
static const QRegExp VERSION_REXP("^(APM:Copter|APM:Plane|APM:Rover|ArduCopter|ArduPlane|ArduRover) +[vV](\\d*)\\.*(\\d*)*\\.*(\\d*)*");

// minimum firmware versions that don't suffer from mavlink severity inversion bug.
// https://github.com/diydrones/apm_planner/issues/788
static const QString MIN_COPTER_VERSION_WITH_CORRECT_SEVERITY_MSGS("APM:Copter V3.4.0");
static const QString MIN_PLANE_VERSION_WITH_CORRECT_SEVERITY_MSGS("APM:Plane V3.4.0");
static const QString MIN_ROVER_VERSION_WITH_CORRECT_SEVERITY_MSGS("APM:Rover V2.6.0");


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

APMFirmwarePlugin::APMFirmwarePlugin(void)
{
     _textSeverityAdjustmentNeeded = false;
}

bool APMFirmwarePlugin::isCapable(FirmwareCapabilities capabilities)
{
    return (capabilities & SetFlightModeCapability) == capabilities;
}

QList<VehicleComponent*> APMFirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);
    
    return QList<VehicleComponent*>();
}

QStringList APMFirmwarePlugin::flightModes(void)
{   
    QStringList flightModesList;
    foreach (const APMCustomMode& customMode, _supportedModes) {
        if (customMode.canBeSet()) {
            flightModesList << customMode.modeString();
        }
    }
    return flightModesList;
}

QString APMFirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode)
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

int APMFirmwarePlugin::manualControlReservedButtonCount(void)
{
    // We don't know whether the firmware is going to used any of these buttons.
    // So reserve them all.
    return -1;
}

void APMFirmwarePlugin::adjustMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    //-- Don't process messages to/from UDP Bridge. It doesn't suffer from these issues
    if (message->compid == MAV_COMP_ID_UDP_BRIDGE) {
        return;
    }

    if (message->msgid == MAVLINK_MSG_ID_PARAM_VALUE) {
        mavlink_param_value_t paramValue;
        mavlink_param_union_t paramUnion;
        
        // APM stack passes all parameter values in mavlink_param_union_t.param_float no matter what
        // type they are. Fix that up to correct usage.
        
        mavlink_msg_param_value_decode(message, &paramValue);
        
        switch (paramValue.param_type) {
            case MAV_PARAM_TYPE_UINT8:
                paramUnion.param_uint8 = (uint8_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_INT8:
                paramUnion.param_int8 = (int8_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_UINT16:
                paramUnion.param_uint16 = (uint16_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_INT16:
                paramUnion.param_int16 = (int16_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_UINT32:
                paramUnion.param_uint32 = (uint32_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_INT32:
                paramUnion.param_int32 = (int32_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_REAL32:
                paramUnion.param_float = paramValue.param_value;
                break;
            default:
                qCCritical(APMFirmwarePluginLog) << "Invalid/Unsupported data type used in parameter:" << paramValue.param_type;
        }
        
        paramValue.param_value = paramUnion.param_float;
        
        mavlink_msg_param_value_encode(message->sysid, message->compid, message, &paramValue);
        
    } else if (message->msgid == MAVLINK_MSG_ID_PARAM_SET) {
        mavlink_param_set_t     paramSet;
        mavlink_param_union_t   paramUnion;
        
        // APM stack passes all parameter values in mavlink_param_union_t.param_float no matter what
        // type they are. Fix it back to the wrong way on the way out.
        
        mavlink_msg_param_set_decode(message, &paramSet);
        
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
        
        mavlink_msg_param_set_encode(message->sysid, message->compid, message, &paramSet);
    }

    if (message->msgid == MAVLINK_MSG_ID_STATUSTEXT) {
        QString messageText;

        mavlink_statustext_t statusText;
        mavlink_msg_statustext_decode(message, &statusText);

        if (!_firmwareVersion.isValid() || statusText.severity < MAV_SEVERITY_NOTICE) {
            messageText = _getMessageText(message);
            qCDebug(APMFirmwarePluginLog) << messageText;

            if (!_firmwareVersion.isValid()) {
                // if don't know firmwareVersion yet, try and see if this message contains it
                if (messageText.contains(APM_COPTER_REXP) || messageText.contains(APM_PLANE_REXP) || messageText.contains(APM_ROVER_REXP)) {
                    // found version string
                    _firmwareVersion = APMFirmwareVersion(messageText);
                    _textSeverityAdjustmentNeeded = _isTextSeverityAdjustmentNeeded(_firmwareVersion);

                    if (!_firmwareVersion.isBeta() && !_firmwareVersion.isDev()) {
                        int supportedMajorNumber = -1;
                        int supportedMinorNumber = -1;

                        switch (vehicle->vehicleType()) {
                        case MAV_TYPE_FIXED_WING:
                            supportedMajorNumber = 3;
                            supportedMinorNumber = 2;
                            break;
                        case MAV_TYPE_QUADROTOR:
                        case MAV_TYPE_COAXIAL:
                        case MAV_TYPE_HELICOPTER:
                        case MAV_TYPE_SUBMARINE:
                        case MAV_TYPE_HEXAROTOR:
                        case MAV_TYPE_OCTOROTOR:
                        case MAV_TYPE_TRICOPTER:
                            supportedMajorNumber = 3;
                            supportedMinorNumber = 2;
                            break;
                        default:
                            break;
                        }

                        if (supportedMajorNumber != -1) {
                            if (_firmwareVersion.majorNumber() < supportedMajorNumber || _firmwareVersion.minorNumber() < supportedMinorNumber) {
                                qgcApp()->showMessage(QString("QGroundControl fully supports Version %1.%2 and above. You are using a version prior to that. This combination is untested, you may run into unpredictable results.").arg(supportedMajorNumber).arg(supportedMinorNumber));
                            }
                        }
                    }
                }
            }

            // APM user facing calibration messages come through as high severity, we need to parse them out
            // and lower the severity on them so that they don't pop in the users face.

            if (messageText.contains("Place vehicle") || messageText.contains("Calibration successful")) {
                _adjustCalibrationMessageSeverity(message);
                return;
            }
        }

        // adjust mesasge if needed
        if (_textSeverityAdjustmentNeeded) {
            _adjustSeverity(message);
        }

        if (messageText.isEmpty()) {
            messageText = _getMessageText(message);
        }

        // The following messages are incorrectly labeled as warning message.
        // Fixed in newer firmware (unreleased at this point), but still in older firmware.
        if (messageText.contains(APM_COPTER_REXP) || messageText.contains(APM_PLANE_REXP) || messageText.contains(APM_ROVER_REXP) ||
                messageText.contains(APM_PX4NUTTX_REXP) || messageText.contains(APM_FRAME_REXP) || messageText.contains(APM_SYSID_REXP)) {
            _setInfoSeverity(message);
        }
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

    mavlink_msg_statustext_encode(message->sysid, message->compid, message, &statusText);
}

void APMFirmwarePlugin::_setInfoSeverity(mavlink_message_t* message) const
{
    mavlink_statustext_t statusText;
    mavlink_msg_statustext_decode(message, &statusText);

    statusText.severity = MAV_SEVERITY_INFO;
    mavlink_msg_statustext_encode(message->sysid, message->compid, message, &statusText);
}

void APMFirmwarePlugin::_adjustCalibrationMessageSeverity(mavlink_message_t* message) const
{
    mavlink_statustext_t statusText;
    mavlink_msg_statustext_decode(message, &statusText);
    statusText.severity = MAV_SEVERITY_INFO;
    mavlink_msg_statustext_encode(message->sysid, message->compid, message, &statusText);
}

void APMFirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    // Streams are not started automatically on APM stack
    vehicle->requestDataStream(MAV_DATA_STREAM_RAW_SENSORS,        2);
    vehicle->requestDataStream(MAV_DATA_STREAM_EXTENDED_STATUS,    2);
    vehicle->requestDataStream(MAV_DATA_STREAM_RC_CHANNELS,        2);
    vehicle->requestDataStream(MAV_DATA_STREAM_POSITION,           3);
    vehicle->requestDataStream(MAV_DATA_STREAM_EXTRA1,             10);
    vehicle->requestDataStream(MAV_DATA_STREAM_EXTRA2,             10);
    vehicle->requestDataStream(MAV_DATA_STREAM_EXTRA3,             3);
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

void APMFirmwarePlugin::addMetaDataToFact(Fact* fact, MAV_TYPE vehicleType)
{
    _parameterMetaData.addMetaDataToFact(fact, vehicleType);
}


QList<MAV_CMD> APMFirmwarePlugin::supportedMissionCommands(void)
{
    QList<MAV_CMD> list;

    list << MAV_CMD_NAV_WAYPOINT << MAV_CMD_NAV_SPLINE_WAYPOINT
         << MAV_CMD_NAV_LOITER_UNLIM << MAV_CMD_NAV_LOITER_TURNS << MAV_CMD_NAV_LOITER_TIME << MAV_CMD_NAV_LOITER_TO_ALT
         << MAV_CMD_NAV_RETURN_TO_LAUNCH << MAV_CMD_NAV_LAND << MAV_CMD_NAV_TAKEOFF
         << MAV_CMD_NAV_GUIDED_ENABLE
         << MAV_CMD_DO_SET_ROI << MAV_CMD_DO_GUIDED_LIMITS << MAV_CMD_DO_JUMP << MAV_CMD_DO_CHANGE_SPEED << MAV_CMD_DO_SET_CAM_TRIGG_DIST
         << MAV_CMD_DO_SET_RELAY << MAV_CMD_DO_REPEAT_RELAY
         << MAV_CMD_DO_SET_SERVO << MAV_CMD_DO_REPEAT_SERVO
         << MAV_CMD_DO_DIGICAM_CONFIGURE << MAV_CMD_DO_DIGICAM_CONTROL
         << MAV_CMD_DO_MOUNT_CONTROL
         << MAV_CMD_DO_SET_HOME
         << MAV_CMD_DO_LAND_START
         << MAV_CMD_DO_FENCE_ENABLE << MAV_CMD_DO_PARACHUTE << MAV_CMD_DO_INVERTED_FLIGHT << MAV_CMD_DO_GRIPPER
         << MAV_CMD_CONDITION_DELAY  << MAV_CMD_CONDITION_CHANGE_ALT << MAV_CMD_CONDITION_DISTANCE << MAV_CMD_CONDITION_YAW
         << MAV_CMD_NAV_VTOL_TAKEOFF << MAV_CMD_NAV_VTOL_LAND
         << MAV_CMD_NAV_CONTINUE_AND_CHANGE_ALT;

    return list;
}

void APMFirmwarePlugin::missionCommandOverrides(QString& commonJsonFilename, QString& fixedWingJsonFilename, QString& multiRotorJsonFilename) const
{
    commonJsonFilename = QStringLiteral(":/json/APM/MavCmdInfoCommon.json");
    fixedWingJsonFilename = QStringLiteral(":/json/APM/MavCmdInfoFixedWing.json");
    multiRotorJsonFilename = QStringLiteral(":/json/APM/MavCmdInfoMultiRotor.json");
}
