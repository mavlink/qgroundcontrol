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
        { FOLLOW,       "Follow Vehicle" },
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
        APMCopterMode(APMCopterMode::FOLLOW,        true),
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

void ArduCopterFirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showMessage(QStringLiteral("Unable to go to location, vehicle position not known."));
        return;
    }

    setGuidedMode(vehicle, true);

    if (vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {

        mavlink_message_t msg;
        mavlink_set_position_target_global_int_t cmd;

        memset(&cmd, 0, sizeof(cmd));

        cmd.target_system    = static_cast<uint8_t>(vehicle->id());
        cmd.target_component = static_cast<uint8_t>(vehicle->defaultComponentId());
        cmd.coordinate_frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
        cmd.type_mask = 0x0DF8; // Position Only
        cmd.lat_int = static_cast<int>(gotoCoord.latitude()*1e7);
        cmd.lon_int = static_cast<int>(gotoCoord.longitude()*1e7);
        cmd.alt = vehicle->altitudeRelative()->rawValue().toFloat();

        MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();

        mavlink_msg_set_position_target_global_int_encode_chan(
                static_cast<uint8_t>(mavlink->getSystemId()),
                static_cast<uint8_t>(mavlink->getComponentId()),
                vehicle->priorityLink()->mavlinkChannel(),
                &msg,
                &cmd);

        vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
    }
    else{
        qgcApp()->showMessage(QStringLiteral("Unable to go to location, vehicle cannot receive INT command."));
        return;
    }

}
