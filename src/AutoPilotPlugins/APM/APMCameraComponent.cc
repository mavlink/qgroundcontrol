/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "APMCameraComponent.h"
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
    return tr("Camera setup is used to adjust camera and gimbal settings.");
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
    if (_vehicle->sub()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMCameraSubComponent.qml"));
    }
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMCameraComponent.qml"));

}

QUrl APMCameraComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMCameraComponentSummary.qml"));
}
