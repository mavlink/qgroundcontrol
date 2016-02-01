/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "AirframeComponent.h"
#include "QGCQmlWidgetHolder.h"

#if 0
// Broken by latest mavlink module changes. Not used yet. Comment out for now.
// Discussing mavlink fix.
struct mavType {
    int         type;
    const char* description;
};

/// @brief Used to translate from MAV_TYPE_* id to user string
static const struct mavType mavTypeInfo[] = {
    { MAV_TYPE_GENERIC,             "Generic" },
    { MAV_TYPE_FIXED_WING,          "Fixed Wing" },
    { MAV_TYPE_QUADROTOR,           "Quadrotor" },
    { MAV_TYPE_COAXIAL,             "Coaxial" },
    { MAV_TYPE_HELICOPTER,          "Helicopter"},
    { MAV_TYPE_ANTENNA_TRACKER,     "Antenna Tracker" },
    { MAV_TYPE_GCS,                 "Ground Control Station" },
    { MAV_TYPE_AIRSHIP,             "Airship" },
    { MAV_TYPE_FREE_BALLOON,        "Free Balloon" },
    { MAV_TYPE_ROCKET,              "Rocket" },
    { MAV_TYPE_GROUND_ROVER,        "Ground Rover" },
    { MAV_TYPE_SURFACE_BOAT,        "Boat" },
    { MAV_TYPE_SUBMARINE,           "Submarine" },
    { MAV_TYPE_HEXAROTOR,           "Hexarotor" },
    { MAV_TYPE_OCTOROTOR,           "Octorotor" },
    { MAV_TYPE_TRICOPTER,           "Tricopter" },
    { MAV_TYPE_FLAPPING_WING,       "Flapping Wing" },
    { MAV_TYPE_KITE,                "Kite" },
    { MAV_TYPE_ONBOARD_CONTROLLER,  "Onbard companion controller" },
    { MAV_TYPE_VTOL_DUOROTOR,       "Two-rotor VTOL" },
    { MAV_TYPE_VTOL_QUADROTOR,      "Quad-rotor VTOL" },
    { MAV_TYPE_VTOL_RESERVED1,      "Reserved" },
    { MAV_TYPE_VTOL_RESERVED2,      "Reserved" },
    { MAV_TYPE_VTOL_RESERVED3,      "Reserved" },
    { MAV_TYPE_VTOL_RESERVED4,      "Reserved" },
    { MAV_TYPE_VTOL_RESERVED5,      "Reserved" },
    { MAV_TYPE_GIMBAL,              "Gimbal" },
};
static size_t cMavTypes = sizeof(mavTypeInfo) / sizeof(mavTypeInfo[0]);
#endif

AirframeComponent::AirframeComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Airframe"))
{
#if 0
    // Broken by latest mavlink module changes. Not used yet. Comment out for now.
    // Discussing mavlink fix.
    Q_UNUSED(mavTypeInfo);  // Keeping this around for later use
    
    // Validate that our mavTypeInfo array hasn't gotten out of sync
    
    qDebug() << cMavTypes << MAV_TYPE_ENUM_END;
    Q_ASSERT(cMavTypes == MAV_TYPE_ENUM_END);
    
    static const int mavTypes[] = {
        MAV_TYPE_GENERIC,
        MAV_TYPE_FIXED_WING,
        MAV_TYPE_QUADROTOR,
        MAV_TYPE_COAXIAL,
        MAV_TYPE_HELICOPTER,
        MAV_TYPE_ANTENNA_TRACKER,
        MAV_TYPE_GCS,
        MAV_TYPE_AIRSHIP,
        MAV_TYPE_FREE_BALLOON,
        MAV_TYPE_ROCKET,
        MAV_TYPE_GROUND_ROVER,
        MAV_TYPE_SURFACE_BOAT,
        MAV_TYPE_SUBMARINE,
        MAV_TYPE_HEXAROTOR,
        MAV_TYPE_OCTOROTOR,
        MAV_TYPE_TRICOPTER,
        MAV_TYPE_FLAPPING_WING,
        MAV_TYPE_KITE,
        MAV_TYPE_ONBOARD_CONTROLLER,
        MAV_TYPE_VTOL_DUOROTOR,
        MAV_TYPE_VTOL_QUADROTOR,
        MAV_TYPE_VTOL_RESERVED1,
        MAV_TYPE_VTOL_RESERVED2,
        MAV_TYPE_VTOL_RESERVED3,
        MAV_TYPE_VTOL_RESERVED4,
        MAV_TYPE_VTOL_RESERVED5,
        MAV_TYPE_GIMBAL,
    };
    Q_UNUSED(mavTypes); // Keeping this around for later use
    
    for (size_t i=0; i<cMavTypes; i++) {
        Q_ASSERT(mavTypeInfo[i].type == mavTypes[i]);
    }
#endif
}

QString AirframeComponent::name(void) const
{
    return _name;
}

QString AirframeComponent::description(void) const
{
    return tr("The Airframe Component is used to select the airframe which matches your vehicle. "
              "This will in turn set up the various tuning values for flight parameters.");
}

QString AirframeComponent::iconResource(void) const
{
    return "/qmlimages/AirframeComponentIcon.png";
}

bool AirframeComponent::requiresSetup(void) const
{
    return true;
}

bool AirframeComponent::setupComplete(void) const
{
    return _autopilot->getParameterFact(FactSystem::defaultComponentId, "SYS_AUTOSTART")->rawValue().toInt() != 0;
}

QStringList AirframeComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList("SYS_AUTOSTART");
}

QUrl AirframeComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/AirframeComponent.qml");
}

QUrl AirframeComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/AirframeComponentSummary.qml");
}

QString AirframeComponent::prerequisiteSetup(void) const
{
    return QString();
}
