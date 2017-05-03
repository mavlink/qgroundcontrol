/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ArduRoverFirmwarePlugin.h"

bool ArduRoverFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduRoverFirmwarePlugin::_remapParamName;

APMRoverMode::APMRoverMode(uint32_t mode, bool settable)
    : APMCustomMode(mode, settable)
{
    QMap<uint32_t,QString> enumToString;
    enumToString.insert(MANUAL,         "Manual");
    enumToString.insert(LEARNING,       "Learning");
    enumToString.insert(STEERING,       "Steering");
    enumToString.insert(HOLD,           "Hold");
    enumToString.insert(AUTO,           "Auto");
    enumToString.insert(RTL,            "RTL");
    enumToString.insert(GUIDED,         "Guided");
    enumToString.insert(INITIALIZING,   "Initializing");

    setEnumToStringMapping(enumToString);
}

ArduRoverFirmwarePlugin::ArduRoverFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMRoverMode(APMRoverMode::MANUAL       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::LEARNING     ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::STEERING     ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::HOLD         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::AUTO         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::RTL          ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::GUIDED       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::INITIALIZING ,false);
    setSupportedModes(supportedFlightModes);

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_2 = _remapParamName[3][2];

        remapV3_2["SERVO5_FUNCTION"] = QStringLiteral("RC5_FUNCTION");
        remapV3_2["SERVO6_FUNCTION"] = QStringLiteral("RC6_FUNCTION");
        remapV3_2["SERVO7_FUNCTION"] = QStringLiteral("RC7_FUNCTION");
        remapV3_2["SERVO8_FUNCTION"] = QStringLiteral("RC8_FUNCTION");
        remapV3_2["SERVO9_FUNCTION"] = QStringLiteral("RC9_FUNCTION");
        remapV3_2["SERVO10_FUNCTION"] = QStringLiteral("RC10_FUNCTION");
        remapV3_2["SERVO11_FUNCTION"] = QStringLiteral("RC11_FUNCTION");
        remapV3_2["SERVO12_FUNCTION"] = QStringLiteral("RC12_FUNCTION");
        remapV3_2["SERVO13_FUNCTION"] = QStringLiteral("RC13_FUNCTION");
        remapV3_2["SERVO14_FUNCTION"] = QStringLiteral("RC14_FUNCTION");

        _remapParamNameIntialized = true;
    }
}

int ArduRoverFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.2
    return majorVersionNumber == 3 ? 2 : Vehicle::versionNotSetValue;
}

