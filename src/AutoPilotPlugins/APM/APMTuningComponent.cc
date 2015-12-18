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

#include "APMTuningComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"

APMTuningComponent::APMTuningComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : APMComponent(vehicle, autopilot, parent)
    , _name("Tuning")
{
}

QString APMTuningComponent::name(void) const
{
    return _name;
}

QString APMTuningComponent::description(void) const
{
    return tr("The Tuning Component is used to tune the flight characteristics of the Vehicle.");
}

QString APMTuningComponent::iconResource(void) const
{
    return "/qmlimages/subMenuButtonImage.png";
}

bool APMTuningComponent::requiresSetup(void) const
{
    return false;
}

bool APMTuningComponent::setupComplete(void) const
{
    return true;
}

QStringList APMTuningComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMTuningComponent::setupSource(void) const
{
    QString qmlFile;

    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_FIXED_WING:
            qmlFile = "qrc:/qml/APMTuningComponentPlane.qml";
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            qmlFile = "qrc:/qml/APMTuningComponentCopter.qml";
            break;
        case MAV_TYPE_GROUND_ROVER:
            qmlFile = "qrc:/qml/APMTuningComponentRover.qml";
            break;
        default:
            qmlFile = "qrc:/qml/APMNotSupported.qml";
            break;
    }

    return QUrl::fromUserInput(qmlFile);
}

QUrl APMTuningComponent::summaryQmlSource(void) const
{
    return QUrl();
}

QString APMTuningComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }

    return QString();
}
