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
    PX4_CUSTOM_SUB_MODE_AUTO_RTGS
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

/// Tranlates from PX4 custom modes to flight mode names
// FIXME: Doens't handle fixed-wing/multi-rotor name differences
static const struct Modes2Name rgModes2Name[] = {
    { PX4_CUSTOM_MAIN_MODE_MANUAL,      0,                                  "Manual",           true },
    { PX4_CUSTOM_MAIN_MODE_ACRO,        0,                                  "Acro",             true },
    { PX4_CUSTOM_MAIN_MODE_STABILIZED,  0,                                  "Stabilized",       true },
    { PX4_CUSTOM_MAIN_MODE_RATTITUDE,   0,                                  "Rattitude",        true },
    { PX4_CUSTOM_MAIN_MODE_ALTCTL,      0,                                  "Altitude Control", true },
    { PX4_CUSTOM_MAIN_MODE_POSCTL,      0,                                  "Position Control", true },
    { PX4_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                  "Offboard Control", true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_READY,     "Auto Ready",       false },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,   "Taking Off",       false },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LOITER,    "Loiter",           true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_MISSION,   "Mission",          true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTL,       "Return To Land",   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LAND,      "Landing",          false },
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

QString PX4FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode)
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

void PX4FirmwarePlugin::adjustMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(message);

    // PX4 Flight Stack plugin does no message adjustment
}

bool PX4FirmwarePlugin::isCapable(FirmwareCapabilities capabilities)
{
    return (capabilities & (MavCmdPreflightStorageCapability | SetFlightModeCapability)) == capabilities;
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

void PX4FirmwarePlugin::addMetaDataToFact(Fact* fact, MAV_TYPE vehicleType)
{
    _parameterMetaData.addMetaDataToFact(fact, vehicleType);
}

QList<MAV_CMD> PX4FirmwarePlugin::supportedMissionCommands(void)
{
    QList<MAV_CMD> list;

    list << MAV_CMD_NAV_WAYPOINT
         << MAV_CMD_NAV_LOITER_UNLIM << MAV_CMD_NAV_LOITER_TURNS << MAV_CMD_NAV_LOITER_TIME
         << MAV_CMD_NAV_RETURN_TO_LAUNCH << MAV_CMD_NAV_LAND << MAV_CMD_NAV_TAKEOFF
         << MAV_CMD_NAV_ROI
         << MAV_CMD_DO_JUMP
         << MAV_CMD_CONDITION_DELAY
         << MAV_CMD_DO_VTOL_TRANSITION
         << MAV_CMD_DO_DIGICAM_CONTROL;
    return list;
}
