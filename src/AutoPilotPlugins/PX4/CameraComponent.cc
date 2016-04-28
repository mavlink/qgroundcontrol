/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
///     @brief  The Camera VehicleComponent is used to setup the camera modes and hardware
///             configuration to use it.
///     @author Gus Grubba <mavlink@grubba.com>

#include "CameraComponent.h"
#include "PX4AutoPilotPlugin.h"

CameraComponent::CameraComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Camera"))
{
}

QString CameraComponent::name(void) const
{
    return _name;
}

QString CameraComponent::description(void) const
{
    return tr("The Camera is used to setup the camera modes and hardware configuration to use it.");
}

QString CameraComponent::iconResource(void) const
{
    return "/qmlimages/CameraComponentIcon.png";
}

bool CameraComponent::requiresSetup(void) const
{
    return false;
}

bool CameraComponent::setupComplete(void) const
{
    return true;
}

QStringList CameraComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl CameraComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/CameraComponent.qml");
}

QUrl CameraComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/CameraComponentSummary.qml");
}

QString CameraComponent::prerequisiteSetup(void) const
{
    return QString();
}
