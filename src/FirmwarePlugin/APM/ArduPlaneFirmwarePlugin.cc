/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ArduPlaneFirmwarePlugin.h"

bool ArduPlaneFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduPlaneFirmwarePlugin::_remapParamName;

APMPlaneMode::APMPlaneMode(uint32_t mode, bool settable)
    : APMCustomMode(mode, settable)
{
    setEnumToStringMapping({ 
        {MANUAL,         "Manual"},
        {CIRCLE,         "Circle"},
        {STABILIZE,      "Stabilize"},
        {TRAINING,       "Training"},
        {ACRO,           "Acro"},
        {FLY_BY_WIRE_A,  "FBW A"},
        {FLY_BY_WIRE_B,  "FBW B"},
        {CRUISE,         "Cruise"},
        {AUTOTUNE,       "Autotune"},
        {AUTO,           "Auto"},
        {RTL,            "RTL"},
        {LOITER,         "Loiter"},
        {GUIDED,         "Guided"},
        {INITIALIZING,   "Initializing"},
        {QSTABILIZE,     "QuadPlane Stabilize"},
        {QHOVER,         "QuadPlane Hover"},
        {QLOITER,        "QuadPlane Loiter"},
        {QLAND,          "QuadPlane Land"},
        {QRTL,           "QuadPlane RTL"},
    });
}

ArduPlaneFirmwarePlugin::ArduPlaneFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMPlaneMode(APMPlaneMode::MANUAL          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::CIRCLE          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::STABILIZE       ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::TRAINING        ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::ACRO            ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::FLY_BY_WIRE_A   ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::FLY_BY_WIRE_B   ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::CRUISE          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::AUTOTUNE        ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::AUTO            ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::RTL             ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::LOITER          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::GUIDED          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::INITIALIZING    ,false);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QSTABILIZE      ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QHOVER          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QLOITER         ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QLAND           ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QRTL            ,true);
    setSupportedModes(supportedFlightModes);

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_10 = _remapParamName[3][10];

        remapV3_10["BATT_ARM_VOLT"] =    QStringLiteral("ARMING_VOLT_MIN");
        remapV3_10["BATT2_ARM_VOLT"] =   QStringLiteral("ARMING_VOLT2_MIN");

        _remapParamNameIntialized = true;
    }
}

int ArduPlaneFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.10
    return majorVersionNumber == 3 ? 10 : Vehicle::versionNotSetValue;
}
