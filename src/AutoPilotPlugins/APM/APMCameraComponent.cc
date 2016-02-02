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

#include "APMCameraComponent.h"
#include "QGCQmlWidgetHolder.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"

APMCameraComponent::APMCameraComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("Camera"))
{
}

QString APMCameraComponent::name(void) const
{
    return _name;
}

QString APMCameraComponent::description(void) const
{
    return tr("The Camera Component is used to setup camera and gimbal settings.");
}

QString APMCameraComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/CameraComponentIcon.png");
}

bool APMCameraComponent::requiresSetup(void) const
{
    return false;
}

bool APMCameraComponent::setupComplete(void) const
{
    return true;
}

QStringList APMCameraComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMCameraComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMCameraComponent.qml"));
}

QUrl APMCameraComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMCameraComponentSummary.qml"));
}

QString APMCameraComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }

    return QString();
}
