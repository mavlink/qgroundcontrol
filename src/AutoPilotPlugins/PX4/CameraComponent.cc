/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief  The Camera VehicleComponent is used to setup the camera modes and hardware
///             configuration to use it.
///     @author Gus Grubba <gus@auterion.com>

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
    return tr("Camera setup is used to adjust camera and gimbal settings.");
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
