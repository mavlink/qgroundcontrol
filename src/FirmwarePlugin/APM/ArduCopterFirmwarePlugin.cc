/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    QMap<uint32_t,QString> enumToString;
    enumToString.insert(STABILIZE, "Stabilize");
    enumToString.insert(ACRO,      "Acro");
    enumToString.insert(ALT_HOLD,  "Altitude Hold");
    enumToString.insert(AUTO,      "Auto");
    enumToString.insert(GUIDED,    "Guided");
    enumToString.insert(LOITER,    "Loiter");
    enumToString.insert(RTL,       "RTL");
    enumToString.insert(CIRCLE,    "Circle");
    enumToString.insert(LAND,      "Land");
    enumToString.insert(DRIFT,     "Drift");
    enumToString.insert(SPORT,     "Sport");
    enumToString.insert(FLIP,      "Flip");
    enumToString.insert(AUTOTUNE,  "Autotune");
    enumToString.insert(POS_HOLD,  "Position Hold");
    enumToString.insert(BRAKE,     "Brake");
    enumToString.insert(THROW,     "Throw");
    enumToString.insert(AVOID_ADSB,"Avoid ADSB");
    enumToString.insert(GUIDED_NOGPS,"Guided No GPS");
    enumToString.insert(SAFE_RTL,"Smart RTL");


    setEnumToStringMapping(enumToString);
}

ArduCopterFirmwarePlugin::ArduCopterFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMCopterMode(APMCopterMode::STABILIZE ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::ACRO      ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::ALT_HOLD  ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::AUTO      ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::GUIDED    ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::LOITER    ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::RTL       ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::CIRCLE    ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::LAND      ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::DRIFT     ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::SPORT     ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::FLIP      ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::AUTOTUNE  ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::POS_HOLD  ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::BRAKE     ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::THROW     ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::AVOID_ADSB,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::GUIDED_NOGPS,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::SAFE_RTL,true);



    setSupportedModes(supportedFlightModes);

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_4 = _remapParamName[3][4];

        remapV3_4["ATC_ANG_RLL_P"] =    QStringLiteral("STB_RLL_P");
        remapV3_4["ATC_ANG_PIT_P"] =    QStringLiteral("STB_PIT_P");
        remapV3_4["ATC_ANG_YAW_P"] =    QStringLiteral("STB_YAW_P");

        remapV3_4["ATC_RAT_RLL_P"] =    QStringLiteral("RATE_RLL_P");
        remapV3_4["ATC_RAT_RLL_I"] =    QStringLiteral("RATE_RLL_I");
        remapV3_4["ATC_RAT_RLL_IMAX"] = QStringLiteral("RATE_RLL_IMAX");
        remapV3_4["ATC_RAT_RLL_D"] =    QStringLiteral("RATE_RLL_D");
        remapV3_4["ATC_RAT_RLL_FILT"] = QStringLiteral("RATE_RLL_FILT_HZ");

        remapV3_4["ATC_RAT_PIT_P"] =    QStringLiteral("RATE_PIT_P");
        remapV3_4["ATC_RAT_PIT_I"] =    QStringLiteral("RATE_PIT_I");
        remapV3_4["ATC_RAT_PIT_IMAX"] = QStringLiteral("RATE_PIT_IMAX");
        remapV3_4["ATC_RAT_PIT_D"] =    QStringLiteral("RATE_PIT_D");
        remapV3_4["ATC_RAT_PIT_FILT"] = QStringLiteral("RATE_PIT_FILT_HZ");

        remapV3_4["ATC_RAT_YAW_P"] =    QStringLiteral("RATE_YAW_P");
        remapV3_4["ATC_RAT_YAW_I"] =    QStringLiteral("RATE_YAW_I");
        remapV3_4["ATC_RAT_YAW_IMAX"] = QStringLiteral("RATE_YAW_IMAX");
        remapV3_4["ATC_RAT_YAW_D"] =    QStringLiteral("RATE_YAW_D");
        remapV3_4["ATC_RAT_YAW_FILT"] = QStringLiteral("RATE_YAW_FILT_HZ");

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

        remapV3_5["ARMING_VOLT_MIN"] = QStringLiteral("ARMING_MIN_VOLT");
        remapV3_5["ARMING_VOLT2_MIN"] = QStringLiteral("ARMING_MIN_VOLT2");

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

bool ArduCopterFirmwarePlugin::vehicleYawsToNextWaypointInMission(const Vehicle* vehicle) const
{
    if (vehicle->isOfflineEditingVehicle()) {
        return FirmwarePlugin::vehicleYawsToNextWaypointInMission(vehicle);
    } else {
        if (vehicle->multiRotor() && vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, QStringLiteral("WP_YAW_BEHAVIOR"))) {
            Fact* yawMode = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, QStringLiteral("WP_YAW_BEHAVIOR"));
            return yawMode && yawMode->rawValue().toInt() != 0;
        }
    }
    return true;
}

