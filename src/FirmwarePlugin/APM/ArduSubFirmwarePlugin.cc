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

        _remapParamNameIntialized = true;
    }
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
