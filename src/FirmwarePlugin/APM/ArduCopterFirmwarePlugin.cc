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

        _remapParamNameIntialized = true;
    }
}

int ArduCopterFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.5
    return majorVersionNumber == 3 ? 5 : Vehicle::versionNotSetValue;
}

bool ArduCopterFirmwarePlugin::isCapable(const Vehicle* vehicle, FirmwareCapabilities capabilities)
{
    Q_UNUSED(vehicle);

    uint32_t vehicleCapabilities = SetFlightModeCapability | GuidedModeCapability | PauseVehicleCapability;

    return (capabilities & vehicleCapabilities) == capabilities;
}

void ArduCopterFirmwarePlugin::guidedModeRTL(Vehicle* vehicle)
{
    vehicle->setFlightMode("RTL");
}

void ArduCopterFirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    vehicle->setFlightMode("Land");
}

void ArduCopterFirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle)
{
    QString takeoffAltParam("PILOT_TKOFF_ALT");

    if (!vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, takeoffAltParam)) {
        qgcApp()->showMessage(tr("Unable to takeoff, %1 parameter missing.").arg(takeoffAltParam));
        return;
    }
    Fact* takeoffAltFact = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, takeoffAltParam);
    float takeoffAlt = takeoffAltFact->rawValue().toDouble();
    if (takeoffAlt <= 0) {
        takeoffAlt = 2.5;
    } else {
        takeoffAlt /= 100;   // centimeters -> meters
    }

    if (!_armVehicleAndValidate(vehicle)) {
        qgcApp()->showMessage(tr("Unable to takeoff: Vehicle failed to arm."));
        return;
    }

    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_NAV_TAKEOFF,
                            true, // show error
                            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            takeoffAlt);
}

void ArduCopterFirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showMessage(QStringLiteral("Unable to go to location, vehicle position not known."));
        return;
    }

    QGeoCoordinate coordWithAltitude = gotoCoord;
    coordWithAltitude.setAltitude(vehicle->altitudeRelative()->rawValue().toDouble());
    vehicle->missionManager()->writeArduPilotGuidedMissionItem(coordWithAltitude, false /* altChangeOnly */);
}

void ArduCopterFirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showMessage(QStringLiteral("Unable to change altitude, vehicle altitude not known."));
        return;
    }

    setGuidedMode(vehicle, true);

    mavlink_message_t msg;
    mavlink_set_position_target_local_ned_t cmd;

    memset(&cmd, 0, sizeof(cmd));

    cmd.target_system = vehicle->id();
    cmd.target_component = vehicle->defaultComponentId();
    cmd.coordinate_frame = MAV_FRAME_LOCAL_OFFSET_NED;
    cmd.type_mask = 0xFFF8; // Only x/y/z valid
    cmd.x = 0.0f;
    cmd.y = 0.0f;
    cmd.z = -(altitudeChange);

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_set_position_target_local_ned_encode_chan(mavlink->getSystemId(),
                                                          mavlink->getComponentId(),
                                                          vehicle->priorityLink()->mavlinkChannel(),
                                                          &msg,
                                                          &cmd);

    vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
}

void ArduCopterFirmwarePlugin::pauseVehicle(Vehicle* vehicle)
{
    vehicle->setFlightMode("Brake");
}

void ArduCopterFirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    if (guidedMode) {
        vehicle->setFlightMode("Guided");
    } else {
        pauseVehicle(vehicle);
    }
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

QString ArduCopterFirmwarePlugin::geoFenceRadiusParam(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    return QStringLiteral("FENCE_RADIUS");
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
