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

#include <QDebug>

IMPLEMENT_QGC_SINGLETON(PX4FirmwarePlugin, FirmwarePlugin)

enum PX4_CUSTOM_MAIN_MODE {
    PX4_CUSTOM_MAIN_MODE_MANUAL = 1,
    PX4_CUSTOM_MAIN_MODE_ALTCTL,
    PX4_CUSTOM_MAIN_MODE_POSCTL,
    PX4_CUSTOM_MAIN_MODE_AUTO,
    PX4_CUSTOM_MAIN_MODE_ACRO,
    PX4_CUSTOM_MAIN_MODE_OFFBOARD,
    PX4_CUSTOM_MAIN_MODE_STABILIZED,
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
    { PX4_CUSTOM_MAIN_MODE_ALTCTL,      0,                                  "Altitude control", true },
    { PX4_CUSTOM_MAIN_MODE_POSCTL,      0,                                  "Position control", true },
    { PX4_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                  "Offboard control", true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_READY,     "Auto ready",       false },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,   "Taking off",       false },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LOITER,    "Loiter",           true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_MISSION,   "Mission",          true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTL,       "Return to Land",   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LAND,      "Landing",          false },
};


PX4FirmwarePlugin::PX4FirmwarePlugin(QObject* parent) :
    FirmwarePlugin(parent)
{
    
}

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
