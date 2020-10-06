/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
        { MANUAL,           "Manual" },
        { CIRCLE,           "Circle" },
        { STABILIZE,        "Stabilize" },
        { TRAINING,         "Training" },
        { ACRO,             "Acro" },
        { FLY_BY_WIRE_A,    "FBW A" },
        { FLY_BY_WIRE_B,    "FBW B" },
        { CRUISE,           "Cruise" },
        { AUTOTUNE,         "Autotune" },
        { AUTO,             "Auto" },
        { RTL,              "RTL" },
        { LOITER,           "Loiter" },
        { TAKEOFF,          "Takeoff" },
        { AVOID_ADSB,       "Avoid ADSB" },
        { GUIDED,           "Guided" },
        { INITIALIZING,     "Initializing" },
        { QSTABILIZE,       "QuadPlane Stabilize" },
        { QHOVER,           "QuadPlane Hover" },
        { QLOITER,          "QuadPlane Loiter" },
        { QLAND,            "QuadPlane Land" },
        { QRTL,             "QuadPlane RTL" },
        { QAUTOTUNE,        "QuadPlane AutoTune" },
        { QACRO,            "QuadPlane Acro" },
        { THERMAL,          "Thermal"},
    });
}

ArduPlaneFirmwarePlugin::ArduPlaneFirmwarePlugin(void)
{
    setSupportedModes({
        APMPlaneMode(APMPlaneMode::MANUAL,          true),
        APMPlaneMode(APMPlaneMode::CIRCLE,          true),
        APMPlaneMode(APMPlaneMode::STABILIZE,       true),
        APMPlaneMode(APMPlaneMode::TRAINING,        true),
        APMPlaneMode(APMPlaneMode::ACRO,            true),
        APMPlaneMode(APMPlaneMode::FLY_BY_WIRE_A,   true),
        APMPlaneMode(APMPlaneMode::FLY_BY_WIRE_B,   true),
        APMPlaneMode(APMPlaneMode::CRUISE,          true),
        APMPlaneMode(APMPlaneMode::AUTOTUNE,        true),
        APMPlaneMode(APMPlaneMode::AUTO,            true),
        APMPlaneMode(APMPlaneMode::RTL,             true),
        APMPlaneMode(APMPlaneMode::LOITER,          true),
        APMPlaneMode(APMPlaneMode::TAKEOFF,         true),
        APMPlaneMode(APMPlaneMode::AVOID_ADSB,      false),
        APMPlaneMode(APMPlaneMode::GUIDED,          true),
        APMPlaneMode(APMPlaneMode::INITIALIZING,    false),
        APMPlaneMode(APMPlaneMode::QSTABILIZE,      true),
        APMPlaneMode(APMPlaneMode::QHOVER,          true),
        APMPlaneMode(APMPlaneMode::QLOITER,         true),
        APMPlaneMode(APMPlaneMode::QLAND,           true),
        APMPlaneMode(APMPlaneMode::QRTL,            true),
        APMPlaneMode(APMPlaneMode::QAUTOTUNE,       true),
        APMPlaneMode(APMPlaneMode::QACRO,           true),
        APMPlaneMode(APMPlaneMode::THERMAL,         true),
    });

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
