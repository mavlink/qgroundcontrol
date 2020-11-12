/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "ArduCopterFirmwarePlugin.h"
#include "QGCApplication.h"
#include "MissionManager.h"
#include "ParameterManager.h"

bool ArduCopterFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduCopterFirmwarePlugin::_remapParamName;

APMCopterMode::APMCopterMode(uint32_t mode, bool settable) :
    APMCustomMode(mode, settable)
{
    setEnumToStringMapping({
        { STABILIZE,    "Stabilize"},
        { ACRO,         "Acro"},
        { ALT_HOLD,     "Altitude Hold"},
        { AUTO,         "Auto"},
        { GUIDED,       "Guided"},
        { LOITER,       "Loiter"},
        { RTL,          "RTL"},
        { CIRCLE,       "Circle"},
        { LAND,         "Land"},
        { DRIFT,        "Drift"},
        { SPORT,        "Sport"},
        { FLIP,         "Flip"},
        { AUTOTUNE,     "Autotune"},
        { POS_HOLD,     "Position Hold"},
        { BRAKE,        "Brake"},
        { THROW,        "Throw"},
        { AVOID_ADSB,   "Avoid ADSB"},
        { GUIDED_NOGPS, "Guided No GPS"},
        { SMART_RTL,    "Smart RTL"},
        { FLOWHOLD,     "Flow Hold" },
#if 0
    // Follow me not ready for Stable
        { FOLLOW,       "Follow" },
#endif
        { ZIGZAG,       "ZigZag" },
    });
}

ArduCopterFirmwarePlugin::ArduCopterFirmwarePlugin(void)
{
    setSupportedModes({
        APMCopterMode(APMCopterMode::STABILIZE,     true),
        APMCopterMode(APMCopterMode::ACRO,          true),
        APMCopterMode(APMCopterMode::ALT_HOLD,      true),
        APMCopterMode(APMCopterMode::AUTO,          true),
        APMCopterMode(APMCopterMode::GUIDED,        true),
        APMCopterMode(APMCopterMode::LOITER,        true),
        APMCopterMode(APMCopterMode::RTL,           true),
        APMCopterMode(APMCopterMode::CIRCLE,        true),
        APMCopterMode(APMCopterMode::LAND,          true),
        APMCopterMode(APMCopterMode::DRIFT,         true),
        APMCopterMode(APMCopterMode::SPORT,         true),
        APMCopterMode(APMCopterMode::FLIP,          true),
        APMCopterMode(APMCopterMode::AUTOTUNE,      true),
        APMCopterMode(APMCopterMode::POS_HOLD,      true),
        APMCopterMode(APMCopterMode::BRAKE,         true),
        APMCopterMode(APMCopterMode::THROW,         true),
        APMCopterMode(APMCopterMode::AVOID_ADSB,    true),
        APMCopterMode(APMCopterMode::GUIDED_NOGPS,  true),
        APMCopterMode(APMCopterMode::SMART_RTL,     true),
        APMCopterMode(APMCopterMode::FLOWHOLD,      true),
#if 0
    // Follow me not ready for Stable
        APMCopterMode(APMCopterMode::FOLLOW,        true),
#endif
        APMCopterMode(APMCopterMode::ZIGZAG,        true),
    });

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_6 = _remapParamName[3][6];

        remapV3_6["BATT_AMP_PERVLT"] =  QStringLiteral("BATT_AMP_PERVOL");
        remapV3_6["BATT2_AMP_PERVLT"] = QStringLiteral("BATT2_AMP_PERVOL");
        remapV3_6["BATT_LOW_MAH"] =     QStringLiteral("FS_BATT_MAH");
        remapV3_6["BATT_LOW_VOLT"] =    QStringLiteral("FS_BATT_VOLTAGE");
        remapV3_6["BATT_FS_LOW_ACT"] =  QStringLiteral("FS_BATT_ENABLE");
        remapV3_6["PSC_ACCZ_P"] =       QStringLiteral("ACCEL_Z_P");
        remapV3_6["PSC_ACCZ_I"] =       QStringLiteral("ACCEL_Z_I");

        FirmwarePlugin::remapParamNameMap_t& remapV3_7 = _remapParamName[3][7];

        remapV3_7["BATT_ARM_VOLT"] =    QStringLiteral("ARMING_VOLT_MIN");
        remapV3_7["BATT2_ARM_VOLT"] =   QStringLiteral("ARMING_VOLT2_MIN");
        remapV3_7["RC7_OPTION"] =       QStringLiteral("CH7_OPT");
        remapV3_7["RC8_OPTION"] =       QStringLiteral("CH8_OPT");
        remapV3_7["RC9_OPTION"] =       QStringLiteral("CH9_OPT");
        remapV3_7["RC10_OPTION"] =      QStringLiteral("CH10_OPT");
        remapV3_7["RC11_OPTION"] =      QStringLiteral("CH11_OPT");
        remapV3_7["RC12_OPTION"] =      QStringLiteral("CH12_OPT");

        FirmwarePlugin::remapParamNameMap_t& remapV4_0 = _remapParamName[4][0];

        remapV4_0["TUNE_MIN"] = QStringLiteral("TUNE_HIGH");
        remapV3_7["TUNE_MAX"] = QStringLiteral("TUNE_LOW");

        _remapParamNameIntialized = true;
    }
}

int ArduCopterFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.7
    return majorVersionNumber == 3 ? 7 : Vehicle::versionNotSetValue;
}

void ArduCopterFirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    _setFlightModeAndValidate(vehicle, "Land");
}

bool ArduCopterFirmwarePlugin::multiRotorCoaxialMotors(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    return _coaxialMotors;
}

bool ArduCopterFirmwarePlugin::multiRotorXConfig(Vehicle* vehicle)
{
    return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "FRAME")->rawValue().toInt() != 0;
}

#if 0
    // Follow me not ready for Stable
void ArduCopterFirmwarePlugin::sendGCSMotionReport(Vehicle* vehicle, FollowMe::GCSMotionReport& motionReport, uint8_t estimatationCapabilities)
{
    _sendGCSMotionReport(vehicle, motionReport, estimatationCapabilities);
}
#endif
