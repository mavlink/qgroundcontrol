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

#include "APMAirframeComponent.h"
#include "ArduCopterFirmwarePlugin.h"

APMAirframeComponent::APMAirframeComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _requiresFrameSetup(false)
    , _name("Airframe")
{
    if (qobject_cast<ArduCopterFirmwarePlugin*>(_vehicle->firmwarePlugin()) != NULL) {
        _requiresFrameSetup = true;
        MAV_TYPE vehicleType = vehicle->vehicleType();
        if (vehicleType == MAV_TYPE_TRICOPTER || vehicleType == MAV_TYPE_HELICOPTER) {
            _requiresFrameSetup = false;
        }
    }
}

QString APMAirframeComponent::name(void) const
{
    return _name;
}

QString APMAirframeComponent::description(void) const
{
    return tr("The Airframe Component is used to select the airframe which matches your vehicle. "
              "This will in turn set up the various tuning values for flight parameters.");
}

QString APMAirframeComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/AirframeComponentIcon.png");
}

bool APMAirframeComponent::requiresSetup(void) const
{
    return _requiresFrameSetup;
}

bool APMAirframeComponent::setupComplete(void) const
{
    if (_requiresFrameSetup) {
        return _autopilot->getParameterFact(FactSystem::defaultComponentId, QStringLiteral("FRAME"))->rawValue().toInt() >= 0;
    } else {
        return true;
    }
}

QStringList APMAirframeComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;

    if (_requiresFrameSetup) {
        list << QStringLiteral("FRAME");
    }

    return list;
}

QUrl APMAirframeComponent::setupSource(void) const
{
    if (_requiresFrameSetup) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMAirframeComponent.qml"));
    } else {
        return QUrl();
    }
}

QUrl APMAirframeComponent::summaryQmlSource(void) const
{
    if (_requiresFrameSetup) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMAirframeComponentSummary.qml"));
    } else {
        return QUrl();
    }
}

QString APMAirframeComponent::prerequisiteSetup(void) const
{
    return QString();
}
