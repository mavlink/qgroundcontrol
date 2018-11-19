/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ArduRoverFirmwarePlugin.h"
#include "QGCApplication.h"

bool ArduRoverFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduRoverFirmwarePlugin::_remapParamName;

APMRoverMode::APMRoverMode(uint32_t mode, bool settable)
    : APMCustomMode(mode, settable)
{
    QMap<uint32_t,QString> enumToString;
    enumToString.insert(MANUAL,         "Manual");
    enumToString.insert(ACRO,           "Acro");
    enumToString.insert(STEERING,       "Steering");
    enumToString.insert(HOLD,           "Hold");
    enumToString.insert(LOITER,         "Loiter");
    enumToString.insert(SIMPLE,         "Simple");
    enumToString.insert(AUTO,           "Auto");
    enumToString.insert(RTL,            "RTL");
    enumToString.insert(SMART_RTL,      "Smart RTL");
    enumToString.insert(GUIDED,         "Guided");
    enumToString.insert(INITIALIZING,   "Initializing");

    setEnumToStringMapping(enumToString);
}

ArduRoverFirmwarePlugin::ArduRoverFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMRoverMode(APMRoverMode::MANUAL       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::ACRO         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::STEERING     ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::HOLD         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::LOITER       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::SIMPLE       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::AUTO         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::RTL          ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::SMART_RTL    ,true);
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

        remapV3_2["SERVO5_MIN"] = QStringLiteral("RC5_MIN");
        remapV3_2["SERVO6_MIN"] = QStringLiteral("RC6_MIN");
        remapV3_2["SERVO7_MIN"] = QStringLiteral("RC7_MIN");
        remapV3_2["SERVO8_MIN"] = QStringLiteral("RC8_MIN");
        remapV3_2["SERVO9_MIN"] = QStringLiteral("RC9_MIN");
        remapV3_2["SERVO10_MIN"] = QStringLiteral("RC10_MIN");
        remapV3_2["SERVO11_MIN"] = QStringLiteral("RC11_MIN");
        remapV3_2["SERVO12_MIN"] = QStringLiteral("RC12_MIN");
        remapV3_2["SERVO13_MIN"] = QStringLiteral("RC13_MIN");
        remapV3_2["SERVO14_MIN"] = QStringLiteral("RC14_MIN");

        remapV3_2["SERVO5_MAX"] = QStringLiteral("RC5_MAX");
        remapV3_2["SERVO6_MAX"] = QStringLiteral("RC6_MAX");
        remapV3_2["SERVO7_MAX"] = QStringLiteral("RC7_MAX");
        remapV3_2["SERVO8_MAX"] = QStringLiteral("RC8_MAX");
        remapV3_2["SERVO9_MAX"] = QStringLiteral("RC9_MAX");
        remapV3_2["SERVO10_MAX"] = QStringLiteral("RC10_MAX");
        remapV3_2["SERVO11_MAX"] = QStringLiteral("RC11_MAX");
        remapV3_2["SERVO12_MAX"] = QStringLiteral("RC12_MAX");
        remapV3_2["SERVO13_MAX"] = QStringLiteral("RC13_MAX");
        remapV3_2["SERVO14_MAX"] = QStringLiteral("RC14_MAX");

        remapV3_2["SERVO5_REVERSED"] = QStringLiteral("RC5_REVERSED");
        remapV3_2["SERVO6_REVERSED"] = QStringLiteral("RC6_REVERSED");
        remapV3_2["SERVO7_REVERSED"] = QStringLiteral("RC7_REVERSED");
        remapV3_2["SERVO8_REVERSED"] = QStringLiteral("RC8_REVERSED");
        remapV3_2["SERVO9_REVERSED"] = QStringLiteral("RC9_REVERSED");
        remapV3_2["SERVO10_REVERSED"] = QStringLiteral("RC10_REVERSED");
        remapV3_2["SERVO11_REVERSED"] = QStringLiteral("RC11_REVERSED");
        remapV3_2["SERVO12_REVERSED"] = QStringLiteral("RC12_REVERSED");
        remapV3_2["SERVO13_REVERSED"] = QStringLiteral("RC13_REVERSED");
        remapV3_2["SERVO14_REVERSED"] = QStringLiteral("RC14_REVERSED");

        remapV3_2["ARMING_VOLT_MIN"] = QStringLiteral("ARMING_MIN_VOLT");
        remapV3_2["ARMING_VOLT2_MIN"] = QStringLiteral("ARMING_MIN_VOLT2");

        FirmwarePlugin::remapParamNameMap_t& remapV3_5 = _remapParamName[3][5];

        remapV3_5["BATT_ARM_VOLT"] =    QStringLiteral("ARMING_VOLT_MIN");
        remapV3_5["BATT2_ARM_VOLT"] =   QStringLiteral("ARMING_VOLT2_MIN");

        _remapParamNameIntialized = true;
    }
}

int ArduRoverFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.5
    return majorVersionNumber == 3 ? 5 : Vehicle::versionNotSetValue;
}

void ArduRoverFirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(altitudeChange);

    qgcApp()->showMessage(QStringLiteral("Change altitude not supported."));
}

bool ArduRoverFirmwarePlugin::supportsNegativeThrust(void)
{
    return true;
}
