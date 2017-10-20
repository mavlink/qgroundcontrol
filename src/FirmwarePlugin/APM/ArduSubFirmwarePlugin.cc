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
///     @author Rustom Jehangir <rusty@bluerobotics.com>

#include "ArduSubFirmwarePlugin.h"

bool ArduSubFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduSubFirmwarePlugin::_remapParamName;

APMSubMode::APMSubMode(uint32_t mode, bool settable) :
    APMCustomMode(mode, settable)
{
    QMap<uint32_t,QString> enumToString;
    enumToString.insert(MANUAL, "Manual");
    enumToString.insert(STABILIZE, "Stabilize");
    enumToString.insert(ACRO, "Acro");
    enumToString.insert(ALT_HOLD,  "Depth Hold");
    enumToString.insert(AUTO, "Auto");
    enumToString.insert(GUIDED, "Guided");
    enumToString.insert(CIRCLE, "Circle");
    enumToString.insert(SURFACE, "Surface");
    enumToString.insert(POSHOLD, "Position Hold");

    setEnumToStringMapping(enumToString);
}

ArduSubFirmwarePlugin::ArduSubFirmwarePlugin(void):
    _infoFactGroup(this)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMSubMode(APMSubMode::MANUAL ,true);
    supportedFlightModes << APMSubMode(APMSubMode::STABILIZE ,true);
    supportedFlightModes << APMSubMode(APMSubMode::ACRO ,true);
    supportedFlightModes << APMSubMode(APMSubMode::ALT_HOLD  ,true);
    supportedFlightModes << APMSubMode(APMSubMode::AUTO ,true);
    supportedFlightModes << APMSubMode(APMSubMode::GUIDED ,true);
    supportedFlightModes << APMSubMode(APMSubMode::CIRCLE ,true);
    supportedFlightModes << APMSubMode(APMSubMode::SURFACE ,false);
    supportedFlightModes << APMSubMode(APMSubMode::POSHOLD ,true);
    setSupportedModes(supportedFlightModes);

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_5 = _remapParamName[3][5];

        remapV3_5["SERVO5_FUNCTION"] = QStringLiteral("RC5_FUNCTION");
        remapV3_5["SERVO6_FUNCTION"] = QStringLiteral("RC6_FUNCTION");
        remapV3_5["SERVO7_FUNCTION"] = QStringLiteral("RC7_FUNCTION");
        remapV3_5["SERVO8_FUNCTION"] = QStringLiteral("RC8_FUNCTION");
        remapV3_5["SERVO9_FUNCTION"] = QStringLiteral("RC9_FUNCTION");
        remapV3_5["SERVO10_FUNCTION"] = QStringLiteral("RC10_FUNCTION");
        remapV3_5["SERVO11_FUNCTION"] = QStringLiteral("RC11_FUNCTION");
        remapV3_5["SERVO12_FUNCTION"] = QStringLiteral("RC12_FUNCTION");
        remapV3_5["SERVO13_FUNCTION"] = QStringLiteral("RC13_FUNCTION");
        remapV3_5["SERVO14_FUNCTION"] = QStringLiteral("RC14_FUNCTION");

        remapV3_5["SERVO5_MIN"] = QStringLiteral("RC5_MIN");
        remapV3_5["SERVO6_MIN"] = QStringLiteral("RC6_MIN");
        remapV3_5["SERVO7_MIN"] = QStringLiteral("RC7_MIN");
        remapV3_5["SERVO8_MIN"] = QStringLiteral("RC8_MIN");
        remapV3_5["SERVO9_MIN"] = QStringLiteral("RC9_MIN");
        remapV3_5["SERVO10_MIN"] = QStringLiteral("RC10_MIN");
        remapV3_5["SERVO11_MIN"] = QStringLiteral("RC11_MIN");
        remapV3_5["SERVO12_MIN"] = QStringLiteral("RC12_MIN");
        remapV3_5["SERVO13_MIN"] = QStringLiteral("RC13_MIN");
        remapV3_5["SERVO14_MIN"] = QStringLiteral("RC14_MIN");

        remapV3_5["SERVO5_MAX"] = QStringLiteral("RC5_MAX");
        remapV3_5["SERVO6_MAX"] = QStringLiteral("RC6_MAX");
        remapV3_5["SERVO7_MAX"] = QStringLiteral("RC7_MAX");
        remapV3_5["SERVO8_MAX"] = QStringLiteral("RC8_MAX");
        remapV3_5["SERVO9_MAX"] = QStringLiteral("RC9_MAX");
        remapV3_5["SERVO10_MAX"] = QStringLiteral("RC10_MAX");
        remapV3_5["SERVO11_MAX"] = QStringLiteral("RC11_MAX");
        remapV3_5["SERVO12_MAX"] = QStringLiteral("RC12_MAX");
        remapV3_5["SERVO13_MAX"] = QStringLiteral("RC13_MAX");
        remapV3_5["SERVO14_MAX"] = QStringLiteral("RC14_MAX");

        remapV3_5["SERVO5_REVERSED"] = QStringLiteral("RC5_REVERSED");
        remapV3_5["SERVO6_REVERSED"] = QStringLiteral("RC6_REVERSED");
        remapV3_5["SERVO7_REVERSED"] = QStringLiteral("RC7_REVERSED");
        remapV3_5["SERVO8_REVERSED"] = QStringLiteral("RC8_REVERSED");
        remapV3_5["SERVO9_REVERSED"] = QStringLiteral("RC9_REVERSED");
        remapV3_5["SERVO10_REVERSED"] = QStringLiteral("RC10_REVERSED");
        remapV3_5["SERVO11_REVERSED"] = QStringLiteral("RC11_REVERSED");
        remapV3_5["SERVO12_REVERSED"] = QStringLiteral("RC12_REVERSED");
        remapV3_5["SERVO13_REVERSED"] = QStringLiteral("RC13_REVERSED");
        remapV3_5["SERVO14_REVERSED"] = QStringLiteral("RC14_REVERSED");

        remapV3_5["FENCE_ALT_MIN"] = QStringLiteral("FENCE_DEPTH_MAX");

        FirmwarePlugin::remapParamNameMap_t& remapV3_6 = _remapParamName[3][6];

        remapV3_6["ARMING_VOLT_MIN"] = QStringLiteral("ARMING_MIN_VOLT");
        remapV3_6["ARMING_VOLT2_MIN"] = QStringLiteral("ARMING_MIN_VOLT2");

        _remapParamNameIntialized = true;
    }

    _nameToFactGroupMap.insert("APMSubInfo", &_infoFactGroup);
}

QList<MAV_CMD> ArduSubFirmwarePlugin::supportedMissionCommands(void)
{
    QList<MAV_CMD> list;

    list << MAV_CMD_NAV_WAYPOINT
         << MAV_CMD_NAV_RETURN_TO_LAUNCH
         << MAV_CMD_NAV_LAND
         << MAV_CMD_NAV_CONTINUE_AND_CHANGE_ALT
         << MAV_CMD_NAV_SPLINE_WAYPOINT
         << MAV_CMD_NAV_GUIDED_ENABLE
         << MAV_CMD_NAV_DELAY
         << MAV_CMD_CONDITION_DELAY << MAV_CMD_CONDITION_DISTANCE << MAV_CMD_CONDITION_YAW
         << MAV_CMD_DO_SET_MODE
         << MAV_CMD_DO_JUMP
         << MAV_CMD_DO_CHANGE_SPEED
         << MAV_CMD_DO_SET_HOME
         << MAV_CMD_DO_SET_RELAY << MAV_CMD_DO_REPEAT_RELAY
         << MAV_CMD_DO_SET_SERVO << MAV_CMD_DO_REPEAT_SERVO
         << MAV_CMD_DO_LAND_START
         << MAV_CMD_DO_SET_ROI
         << MAV_CMD_DO_DIGICAM_CONFIGURE << MAV_CMD_DO_DIGICAM_CONTROL
         << MAV_CMD_DO_MOUNT_CONTROL
         << MAV_CMD_DO_SET_CAM_TRIGG_DIST
         << MAV_CMD_DO_FENCE_ENABLE
         << MAV_CMD_DO_INVERTED_FLIGHT
         << MAV_CMD_DO_GRIPPER
         << MAV_CMD_DO_GUIDED_LIMITS
         << MAV_CMD_DO_AUTOTUNE_ENABLE;

    return list;
}

int ArduSubFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.6
    return majorVersionNumber == 3 ? 6 : Vehicle::versionNotSetValue;
}

int ArduSubFirmwarePlugin::manualControlReservedButtonCount(void)
{
    return 0;
}

void ArduSubFirmwarePlugin::initializeStreamRates(Vehicle* vehicle) {
    vehicle->requestDataStream(MAV_DATA_STREAM_RAW_SENSORS,     2);
    vehicle->requestDataStream(MAV_DATA_STREAM_EXTENDED_STATUS, 2);
    vehicle->requestDataStream(MAV_DATA_STREAM_RC_CHANNELS,     2);
    vehicle->requestDataStream(MAV_DATA_STREAM_POSITION,        3);
    vehicle->requestDataStream(MAV_DATA_STREAM_EXTRA1,          20);
    vehicle->requestDataStream(MAV_DATA_STREAM_EXTRA2,          10);
    vehicle->requestDataStream(MAV_DATA_STREAM_EXTRA3,          3);
}

bool ArduSubFirmwarePlugin::isCapable(const Vehicle* vehicle, FirmwareCapabilities capabilities)
{
    Q_UNUSED(vehicle);
    uint32_t available = SetFlightModeCapability | PauseVehicleCapability;
    return (capabilities & available) == capabilities;
}

bool ArduSubFirmwarePlugin::supportsThrottleModeCenterZero(void)
{
    return false;
}

bool ArduSubFirmwarePlugin::supportsRadio(void)
{
    return false;
}

bool ArduSubFirmwarePlugin::supportsJSButton(void)
{
    return true;
}

bool ArduSubFirmwarePlugin::supportsMotorInterference(void)
{
    return false;
}

const QVariantList& ArduSubFirmwarePlugin::toolBarIndicators(const Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    //-- Sub specific list of indicators (Enter your modified list here)
    if(_toolBarIndicators.size() == 0) {
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/JoystickIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ModeIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ArmedIndicator.qml")));
    }
    return _toolBarIndicators;
}

void ArduSubFirmwarePlugin::_handleNamedValueFloat(mavlink_message_t* message)
{
    mavlink_named_value_float_t value;
    mavlink_msg_named_value_float_decode(message, &value);

    QString name = QString(value.name);

    if (name == "CamTilt") {
        _infoFactGroup.getFact("camera tilt")->setRawValue(value.value * 100);
    } else if (name == "TetherTrn") {
        _infoFactGroup.getFact("tether turns")->setRawValue(value.value);
    } else if (name == "Lights1") {
        _infoFactGroup.getFact("lights 1")->setRawValue(value.value * 100);
    } else if (name == "Lights2") {
        _infoFactGroup.getFact("lights 2")->setRawValue(value.value * 100);
    } else if (name == "PilotGain") {
        _infoFactGroup.getFact("pilot gain")->setRawValue(value.value * 100);
    } else if (name == "InputHold") {
        _infoFactGroup.getFact("input hold")->setRawValue(value.value);
    }
}

void ArduSubFirmwarePlugin::_handleMavlinkMessage(mavlink_message_t* message)
{
    switch (message->msgid) {
    case (MAVLINK_MSG_ID_NAMED_VALUE_FLOAT):
        _handleNamedValueFloat(message);
        break;
    case (MAVLINK_MSG_ID_RANGEFINDER):
    {
        mavlink_rangefinder_t msg;
        mavlink_msg_rangefinder_decode(message, &msg);
        _infoFactGroup.getFact("rangefinder distance")->setRawValue(msg.distance);
        break;
    }
    }
}

bool ArduSubFirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    _handleMavlinkMessage(message);
    return APMFirmwarePlugin::adjustIncomingMavlinkMessage(vehicle, message);
}

QMap<QString, FactGroup*>* ArduSubFirmwarePlugin::factGroups(void) {
    return &_nameToFactGroupMap;
}

const char* APMSubmarineFactGroup::_camTiltFactName             = "camera tilt";
const char* APMSubmarineFactGroup::_tetherTurnsFactName         = "tether turns";
const char* APMSubmarineFactGroup::_lightsLevel1FactName        = "lights 1";
const char* APMSubmarineFactGroup::_lightsLevel2FactName        = "lights 2";
const char* APMSubmarineFactGroup::_pilotGainFactName           = "pilot gain";
const char* APMSubmarineFactGroup::_inputHoldFactName           = "input hold";
const char* APMSubmarineFactGroup::_rangefinderDistanceFactName = "rangefinder distance";

APMSubmarineFactGroup::APMSubmarineFactGroup(QObject* parent)
    : FactGroup(300, ":/json/Vehicle/SubmarineFact.json", parent)
    , _camTiltFact             (0, _camTiltFactName,             FactMetaData::valueTypeDouble)
    , _tetherTurnsFact         (0, _tetherTurnsFactName,         FactMetaData::valueTypeDouble)
    , _lightsLevel1Fact        (0, _lightsLevel1FactName,        FactMetaData::valueTypeDouble)
    , _lightsLevel2Fact        (0, _lightsLevel2FactName,        FactMetaData::valueTypeDouble)
    , _pilotGainFact           (0, _pilotGainFactName,           FactMetaData::valueTypeDouble)
    , _inputHoldFact           (0, _inputHoldFactName,           FactMetaData::valueTypeDouble)
    , _rangefinderDistanceFact (0, _rangefinderDistanceFactName, FactMetaData::valueTypeDouble)
{
    _addFact(&_camTiltFact,             _camTiltFactName);
    _addFact(&_tetherTurnsFact,         _tetherTurnsFactName);
    _addFact(&_lightsLevel1Fact,        _lightsLevel1FactName);
    _addFact(&_lightsLevel2Fact,        _lightsLevel2FactName);
    _addFact(&_pilotGainFact,           _pilotGainFactName);
    _addFact(&_inputHoldFact,           _inputHoldFactName);
    _addFact(&_rangefinderDistanceFact, _rangefinderDistanceFactName);

    // Start out as not available "--.--"
    _camTiltFact.setRawValue             (std::numeric_limits<float>::quiet_NaN());
    _tetherTurnsFact.setRawValue         (std::numeric_limits<float>::quiet_NaN());
    _lightsLevel1Fact.setRawValue        (std::numeric_limits<float>::quiet_NaN());
    _lightsLevel2Fact.setRawValue        (std::numeric_limits<float>::quiet_NaN());
    _pilotGainFact.setRawValue           (std::numeric_limits<float>::quiet_NaN());
    _inputHoldFact.setRawValue           (std::numeric_limits<float>::quiet_NaN());
    _rangefinderDistanceFact.setRawValue (std::numeric_limits<float>::quiet_NaN());

}

QString ArduSubFirmwarePlugin::vehicleImageOpaque(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/qmlimages/subVehicleArrowOpaque.png");
}

QString ArduSubFirmwarePlugin::vehicleImageOutline(const Vehicle* vehicle) const
{
    return vehicleImageOpaque(vehicle);
}
