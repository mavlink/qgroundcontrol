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
    setEnumToStringMapping({
        {MANUAL,         "Manual"},
        {ACRO,           "Acro"},
        {STEERING,       "Steering"},
        {HOLD,           "Hold"},
        {LOITER,         "Loiter"},
        {FOLLOW,         "Follow"},
        {SIMPLE,         "Simple"},
        {AUTO,           "Auto"},
        {RTL,            "RTL"},
        {SMART_RTL,      "Smart RTL"},
        {GUIDED,         "Guided"},
        {INITIALIZING,   "Initializing"},
    });
}

ArduRoverFirmwarePlugin::ArduRoverFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMRoverMode(APMRoverMode::MANUAL       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::ACRO         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::STEERING     ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::HOLD         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::LOITER       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::FOLLOW       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::SIMPLE       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::AUTO         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::RTL          ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::SMART_RTL    ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::GUIDED       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::INITIALIZING ,false);
    setSupportedModes(supportedFlightModes);

    if (!_remapParamNameIntialized) {
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

bool ArduRoverFirmwarePlugin::supportsNegativeThrust(Vehicle* /*vehicle*/)
{
    return true;
}
