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
    enumToString.insert(ALT_HOLD,  "Depth Hold");

    setEnumToStringMapping(enumToString);
}

ArduSubFirmwarePlugin::ArduSubFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMSubMode(APMSubMode::MANUAL ,true);
    supportedFlightModes << APMSubMode(APMSubMode::STABILIZE ,true);
    supportedFlightModes << APMSubMode(APMSubMode::ALT_HOLD  ,true);
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

        _remapParamNameIntialized = true;
    }

    fwFactGroup = new APMSubmarineFactGroup(this);
}

int ArduSubFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.5
    return majorVersionNumber == 3 ? 5 : Vehicle::versionNotSetValue;
}

int ArduSubFirmwarePlugin::manualControlReservedButtonCount(void)
{
    return 0;
}

bool ArduSubFirmwarePlugin::supportsThrottleModeCenterZero(void)
{
    return false;
}

bool ArduSubFirmwarePlugin::supportsManualControl(void)
{
    return true;
}

bool ArduSubFirmwarePlugin::supportsRadio(void)
{
    return false;
}

bool ArduSubFirmwarePlugin::supportsJSButton(void)
{
    return true;
}

bool ArduSubFirmwarePlugin::supportsCalibratePressure(void)
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
        fwFactGroup->getFact("camera tilt")->setRawValue(value.value * 100);
    } else if (name == "TetherTrn") {
        fwFactGroup->getFact("tether turns")->setRawValue(value.value);
    } else if (name == "Lights1") {
        fwFactGroup->getFact("lights 1")->setRawValue(value.value * 100);
    } else if (name == "Lights2") {
        fwFactGroup->getFact("lights 2")->setRawValue(value.value * 100);
    }
}

void ArduSubFirmwarePlugin::_handleMavlinkMessage(mavlink_message_t* message)
{
    switch (message->msgid) {
    case (MAVLINK_MSG_ID_NAMED_VALUE_FLOAT):
        _handleNamedValueFloat(message);
    }
}

bool ArduSubFirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    _handleMavlinkMessage(message);
    return APMFirmwarePlugin::adjustIncomingMavlinkMessage(vehicle, message);
}

const char* APMSubmarineFactGroup::_camTiltFactName = "camera tilt";
const char* APMSubmarineFactGroup::_tetherTurnsFactName = "tether turns";
const char* APMSubmarineFactGroup::_lightsLevel1FactName = "lights 1";
const char* APMSubmarineFactGroup::_lightsLevel2FactName = "lights 2";

APMSubmarineFactGroup::APMSubmarineFactGroup(QObject* parent)
    : FactGroup(300, ":/json/Vehicle/SubmarineFact.json", parent)
    , _camTiltFact       (0, _camTiltFactName,       FactMetaData::valueTypeDouble)
    , _tetherTurnsFact   (0, _tetherTurnsFactName,   FactMetaData::valueTypeDouble)
    , _lightsLevel1Fact  (0, _lightsLevel1FactName,  FactMetaData::valueTypeDouble)
    , _lightsLevel2Fact  (0, _lightsLevel2FactName,  FactMetaData::valueTypeDouble)
{
    _addFact(&_camTiltFact,       _camTiltFactName);
    _addFact(&_tetherTurnsFact,   _tetherTurnsFactName);
    _addFact(&_lightsLevel1Fact,  _lightsLevel1FactName);
    _addFact(&_lightsLevel2Fact,  _lightsLevel2FactName);

    // Start out as not available "--.--"
    _camTiltFact.setRawValue       (std::numeric_limits<float>::quiet_NaN());
    _tetherTurnsFact.setRawValue   (std::numeric_limits<float>::quiet_NaN());
    _lightsLevel1Fact.setRawValue  (std::numeric_limits<float>::quiet_NaN());
    _lightsLevel2Fact.setRawValue  (std::numeric_limits<float>::quiet_NaN());
}
