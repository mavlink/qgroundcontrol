/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#if 0
    // Follow me not ready for Stable
        {FOLLOW,         "Follow"},
#endif
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
    setSupportedModes({
        APMRoverMode(APMRoverMode::MANUAL       ,true),
        APMRoverMode(APMRoverMode::ACRO         ,true),
        APMRoverMode(APMRoverMode::STEERING     ,true),
        APMRoverMode(APMRoverMode::HOLD         ,true),
        APMRoverMode(APMRoverMode::LOITER       ,true),
#if 0
    // Follow me not ready for Stable
        APMRoverMode(APMRoverMode::FOLLOW       ,true),
#endif
        APMRoverMode(APMRoverMode::SIMPLE       ,true),
        APMRoverMode(APMRoverMode::AUTO         ,true),
        APMRoverMode(APMRoverMode::RTL          ,true),
        APMRoverMode(APMRoverMode::SMART_RTL    ,true),
        APMRoverMode(APMRoverMode::GUIDED       ,true),
        APMRoverMode(APMRoverMode::INITIALIZING ,false),
    });

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

void ArduRoverFirmwarePlugin::guidedModeChangeAltitude(Vehicle* /*vehicle*/, double /*altitudeChange*/, bool /*pauseVehicle*/)
{
    qgcApp()->showAppMessage(QStringLiteral("Change altitude not supported."));
}

bool ArduRoverFirmwarePlugin::supportsNegativeThrust(Vehicle* /*vehicle*/)
{
    return true;
}

#if 0
    // Follow me not ready for Stable
void ArduRoverFirmwarePlugin::sendGCSMotionReport(Vehicle* vehicle, FollowMe::GCSMotionReport& motionReport, uint8_t estimatationCapabilities)
{
    _sendGCSMotionReport(vehicle, motionReport, estimatationCapabilities);
}
#endif
