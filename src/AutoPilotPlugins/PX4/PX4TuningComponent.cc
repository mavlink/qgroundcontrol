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

#include "PX4TuningComponent.h"
#include "PX4AutoPilotPlugin.h"
#include "AirframeComponent.h"

PX4TuningComponent::PX4TuningComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name("Tuning")
{
}

QString PX4TuningComponent::name(void) const
{
    return _name;
}

QString PX4TuningComponent::description(void) const
{
    return tr("The Tuning Component is used to tune the flight characteristics of the Vehicle.");
}

QString PX4TuningComponent::iconResource(void) const
{
    return "/qmlimages/TuningComponentIcon.png";
}

bool PX4TuningComponent::requiresSetup(void) const
{
    return false;
}

bool PX4TuningComponent::setupComplete(void) const
{
    return true;
}

QStringList PX4TuningComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl PX4TuningComponent::setupSource(void) const
{
    QString qmlFile;

    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_FIXED_WING:
            qmlFile = "qrc:/qml/PX4TuningComponentPlane.qml";
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            qmlFile = "qrc:/qml/PX4TuningComponentCopter.qml";
            break;
        case MAV_TYPE_VTOL_DUOROTOR:
        case MAV_TYPE_VTOL_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_RESERVED2:
        case MAV_TYPE_VTOL_RESERVED3:
        case MAV_TYPE_VTOL_RESERVED4:
        case MAV_TYPE_VTOL_RESERVED5:
            qmlFile = "qrc:/qml/PX4TuningComponentVTOL.qml";
            break;
        default:
            break;
    }

    return QUrl::fromUserInput(qmlFile);
}

QUrl PX4TuningComponent::summaryQmlSource(void) const
{
    return QUrl();
}

QString PX4TuningComponent::prerequisiteSetup(void) const
{
    PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
    if (plugin) {
        if (!plugin->airframeComponent()->setupComplete()) {
            return plugin->airframeComponent()->name();
        }
    } else {
        qWarning() << "Internal error: plugin cast failed";
    }

    return QString();
}
