/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "PX4FirmwarePlugin.h"
#include "PX4ParameterMetaData.h"
#include "QGCApplication.h"
#include "AutoPilotPlugins/PX4/PX4AutoPilotPlugin.h"    // FIXME: Hack

#include <QDebug>

enum PX4_CUSTOM_MAIN_MODE {
    PX4_CUSTOM_MAIN_MODE_MANUAL = 1,
    PX4_CUSTOM_MAIN_MODE_ALTCTL,
    PX4_CUSTOM_MAIN_MODE_POSCTL,
    PX4_CUSTOM_MAIN_MODE_AUTO,
    PX4_CUSTOM_MAIN_MODE_ACRO,
    PX4_CUSTOM_MAIN_MODE_OFFBOARD,
    PX4_CUSTOM_MAIN_MODE_STABILIZED,
    PX4_CUSTOM_MAIN_MODE_RATTITUDE
};

enum PX4_CUSTOM_SUB_MODE_AUTO {
    PX4_CUSTOM_SUB_MODE_AUTO_READY = 1,
    PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,
    PX4_CUSTOM_SUB_MODE_AUTO_LOITER,
    PX4_CUSTOM_SUB_MODE_AUTO_MISSION,
    PX4_CUSTOM_SUB_MODE_AUTO_RTL,
    PX4_CUSTOM_SUB_MODE_AUTO_LAND,
    PX4_CUSTOM_SUB_MODE_AUTO_RTGS,
    PX4_CUSTOM_SUB_MODE_AUTO_FOLLOW_ME
};

union px4_custom_mode {
    struct {
        uint16_t reserved;
        uint8_t main_mode;
        uint8_t sub_mode;
    };
    uint32_t data;
    float data_float;
};

struct Modes2Name {
    uint8_t     main_mode;
    uint8_t     sub_mode;
    const char* name;       ///< Name for flight mode
    bool        canBeSet;   ///< true: Vehicle can be set to this flight mode
};

const char* PX4FirmwarePlugin::manualFlightMode =       "Manual";
const char* PX4FirmwarePlugin::acroFlightMode =         "Acro";
const char* PX4FirmwarePlugin::stabilizedFlightMode =   "Stabilized";
const char* PX4FirmwarePlugin::rattitudeFlightMode =    "Rattitude";
const char* PX4FirmwarePlugin::altCtlFlightMode =       "Altitude Control";
const char* PX4FirmwarePlugin::posCtlFlightMode =       "Position Control";
const char* PX4FirmwarePlugin::offboardFlightMode =     "Offboard Control";
const char* PX4FirmwarePlugin::readyFlightMode =        "Ready";
const char* PX4FirmwarePlugin::takeoffFlightMode =      "Takeoff";
const char* PX4FirmwarePlugin::pauseFlightMode =        "Hold";
const char* PX4FirmwarePlugin::missionFlightMode =      "Mission";
const char* PX4FirmwarePlugin::rtlFlightMode =          "Return To Land";
const char* PX4FirmwarePlugin::landingFlightMode =      "Landing";
const char* PX4FirmwarePlugin::rtgsFlightMode =         "Return, Link Loss";
const char* PX4FirmwarePlugin::followMeFlightMode =     "Follow Me";

/// Tranlates from PX4 custom modes to flight mode names

static const struct Modes2Name rgModes2Name[] = {
    { PX4_CUSTOM_MAIN_MODE_MANUAL,      0,                                  PX4FirmwarePlugin::manualFlightMode,        true },
    { PX4_CUSTOM_MAIN_MODE_ACRO,        0,                                  PX4FirmwarePlugin::acroFlightMode,          true },
    { PX4_CUSTOM_MAIN_MODE_STABILIZED,  0,                                  PX4FirmwarePlugin::stabilizedFlightMode,    true },
    { PX4_CUSTOM_MAIN_MODE_RATTITUDE,   0,                                  PX4FirmwarePlugin::rattitudeFlightMode,     true },
    { PX4_CUSTOM_MAIN_MODE_ALTCTL,      0,                                  PX4FirmwarePlugin::altCtlFlightMode,        true },
    { PX4_CUSTOM_MAIN_MODE_POSCTL,      0,                                  PX4FirmwarePlugin::posCtlFlightMode,        true },
    { PX4_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                  PX4FirmwarePlugin::offboardFlightMode,      true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_READY,     PX4FirmwarePlugin::readyFlightMode,         false },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,   PX4FirmwarePlugin::takeoffFlightMode,       false },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LOITER,    PX4FirmwarePlugin::pauseFlightMode,         true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_MISSION,   PX4FirmwarePlugin::missionFlightMode,       true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTL,       PX4FirmwarePlugin::rtlFlightMode,           true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LAND,      PX4FirmwarePlugin::landingFlightMode,       false },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTGS,      PX4FirmwarePlugin::rtgsFlightMode,          false },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_FOLLOW_ME, PX4FirmwarePlugin::followMeFlightMode,      true },
};

QList<VehicleComponent*> PX4FirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);

    return QList<VehicleComponent*>();
}

QStringList PX4FirmwarePlugin::flightModes(void)
{
    QStringList flightModes;

    // FIXME: fixed-wing/multi-rotor differences?

    for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
        const struct Modes2Name* pModes2Name = &rgModes2Name[i];

        if (pModes2Name->canBeSet) {
            flightModes += pModes2Name->name;
        }
    }

    return flightModes;
}

QString PX4FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    QString flightMode = "Unknown";

    if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        union px4_custom_mode px4_mode;
        px4_mode.data = custom_mode;

        // FIXME: fixed-wing/multi-rotor differences?

        bool found = false;
        for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
            const struct Modes2Name* pModes2Name = &rgModes2Name[i];

            if (pModes2Name->main_mode == px4_mode.main_mode && pModes2Name->sub_mode == px4_mode.sub_mode) {
                flightMode = pModes2Name->name;
                found = true;
                break;
            }
        }

        if (!found) {
            qWarning() << "Unknown flight mode" << custom_mode;
        }
    } else {
        qWarning() << "PX4 Flight Stack flight mode without custom mode enabled?";
    }

    return flightMode;
}

bool PX4FirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    *base_mode = 0;
    *custom_mode = 0;

    bool found = false;
    for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
        const struct Modes2Name* pModes2Name = &rgModes2Name[i];

        if (flightMode.compare(pModes2Name->name, Qt::CaseInsensitive) == 0) {
            union px4_custom_mode px4_mode;

            px4_mode.data = 0;
            px4_mode.main_mode = pModes2Name->main_mode;
            px4_mode.sub_mode = pModes2Name->sub_mode;

            *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
            *custom_mode = px4_mode.data;

            found = true;
            break;
        }
    }

    if (!found) {
        qWarning() << "Unknown flight Mode" << flightMode;
    }

    return found;
}

int PX4FirmwarePlugin::manualControlReservedButtonCount(void)
{
    return 0;   // 0 buttons reserved for rc switch simulation
}

bool PX4FirmwarePlugin::isCapable(FirmwareCapabilities capabilities)
{
    return (capabilities & (MavCmdPreflightStorageCapability | GuidedModeCapability | SetFlightModeCapability | PauseVehicleCapability)) == capabilities;
}

void PX4FirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);

    // PX4 Flight Stack doesn't need to do any extra work
}

bool PX4FirmwarePlugin::sendHomePositionToVehicle(void)
{
    // PX4 stack does not want home position sent in the first position.
    // Subsequent sequence numbers must be adjusted.
    return false;
}

void PX4FirmwarePlugin::addMetaDataToFact(QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType)
{
    PX4ParameterMetaData* px4MetaData = qobject_cast<PX4ParameterMetaData*>(parameterMetaData);

    if (px4MetaData) {
        px4MetaData->addMetaDataToFact(fact, vehicleType);
    } else {
        qWarning() << "Internal error: pointer passed to PX4FirmwarePlugin::addMetaDataToFact not PX4ParameterMetaData";
    }
}

QList<MAV_CMD> PX4FirmwarePlugin::supportedMissionCommands(void)
{
    QList<MAV_CMD> list;

    list << MAV_CMD_NAV_WAYPOINT
         << MAV_CMD_NAV_LOITER_UNLIM << MAV_CMD_NAV_LOITER_TIME
         << MAV_CMD_NAV_LAND << MAV_CMD_NAV_TAKEOFF
         << MAV_CMD_DO_JUMP
         << MAV_CMD_DO_VTOL_TRANSITION << MAV_CMD_NAV_VTOL_TAKEOFF << MAV_CMD_NAV_VTOL_LAND
         << MAV_CMD_DO_DIGICAM_CONTROL
         << MAV_CMD_DO_SET_CAM_TRIGG_DIST
         << MAV_CMD_DO_SET_SERVO
         << MAV_CMD_DO_CHANGE_SPEED
         << MAV_CMD_NAV_PATHPLANNING;

    return list;
}

void PX4FirmwarePlugin::missionCommandOverrides(QString& commonJsonFilename, QString& fixedWingJsonFilename, QString& multiRotorJsonFilename) const
{
    // No overrides
    commonJsonFilename = QStringLiteral(":/json/PX4/MavCmdInfoCommon.json");
    fixedWingJsonFilename = QStringLiteral(":/json/PX4/MavCmdInfoFixedWing.json");
    multiRotorJsonFilename = QStringLiteral(":/json/PX4/MavCmdInfoMultiRotor.json");
}

QObject* PX4FirmwarePlugin::loadParameterMetaData(const QString& metaDataFile)
{
    PX4ParameterMetaData* metaData = new PX4ParameterMetaData;
    metaData->loadParameterFactMetaDataFile(metaDataFile);
    return metaData;
}

void PX4FirmwarePlugin::pauseVehicle(Vehicle* vehicle)
{
    // kick it into hold mode
    vehicle->setFlightMode(pauseFlightMode);

    // then tell it to loiter at the current position
    // above the takeoff (home) position
    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_DO_REPOSITION;
    cmd.confirmation = 0;
    cmd.param1 = -1.0f;
    cmd.param2 = 0.0;
    cmd.param3 = 0.0f;
    cmd.param4 = NAN;
    cmd.param5 = NAN;
    cmd.param6 = NAN;
    cmd.param7 = NAN;
    cmd.target_system = vehicle->id();
    cmd.target_component = 0;

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_command_long_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &cmd);

    vehicle->sendMessage(msg);
}

void PX4FirmwarePlugin::guidedModeRTL(Vehicle* vehicle)
{
    vehicle->setFlightMode(rtlFlightMode);
}

void PX4FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    vehicle->setFlightMode(landingFlightMode);
}

void PX4FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double altitudeRel)
{
    Q_UNUSED(altitudeRel);
    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showMessage(QStringLiteral("Unable to takeoff, vehicle position not known."));
        return;
    }

    // tell the system first to take off in its internal,
    // airframe specific takeoff action
    vehicle->setFlightMode(takeoffFlightMode);

    // then tell it to loiter at the user-selected location
    // above the takeoff (home) position
    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_DO_REPOSITION;
    cmd.confirmation = 0;
    cmd.param1 = -1.0f;
    cmd.param2 = 0.0;
    cmd.param3 = 0.0f;
    cmd.param4 = NAN;
    cmd.param5 = NAN;
    cmd.param6 = NAN;
    cmd.param7 = vehicle->altitudeAMSL()->rawValue().toDouble() + altitudeRel;
    cmd.target_system = vehicle->id();
    cmd.target_component = 0;

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_command_long_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &cmd);

    vehicle->sendMessage(msg);
}

void PX4FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showMessage(QStringLiteral("Unable to go to location, vehicle position not known."));
        return;
    }

    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_DO_REPOSITION;
    cmd.confirmation = 0;
    cmd.param1 = -1.0f;
    cmd.param2 = MAV_DO_REPOSITION_FLAGS_CHANGE_MODE;
    cmd.param3 = 0.0f;
    cmd.param4 = NAN;
    cmd.param5 = gotoCoord.latitude() * 1e7;
    cmd.param6 = gotoCoord.longitude() * 1e7;
    cmd.param7 = vehicle->altitudeAMSL()->rawValue().toDouble();
    cmd.target_system = vehicle->id();
    cmd.target_component = 0;

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_command_long_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &cmd);

    vehicle->sendMessage(msg);
}

void PX4FirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeRel)
{
    if (qIsNaN(vehicle->altitudeRelative()->rawValue().toDouble())) {
        qgcApp()->showMessage(QStringLiteral("Unable to change altitude, vehicle altitude not known."));
        return;
    }

    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_DO_REPOSITION;
    cmd.confirmation = 0;
    cmd.param1 = -1.0f;
    cmd.param2 = MAV_DO_REPOSITION_FLAGS_CHANGE_MODE;
    cmd.param3 = 0.0f;
    cmd.param4 = NAN;
    cmd.param5 = NAN;
    cmd.param6 = NAN;
    cmd.param7 = vehicle->altitudeAMSL()->rawValue().toDouble() + altitudeRel;
    cmd.target_system = vehicle->id();
    cmd.target_component = 0;

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_command_long_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &cmd);

    vehicle->sendMessage(msg);
}

void PX4FirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    if (guidedMode) {
        vehicle->setFlightMode(pauseFlightMode);
    } else {
        pauseVehicle(vehicle);
    }
}

bool PX4FirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    // Not supported by generic vehicle
    return (vehicle->flightMode() == pauseFlightMode);
}
