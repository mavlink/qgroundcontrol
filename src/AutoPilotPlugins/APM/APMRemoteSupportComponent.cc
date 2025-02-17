/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMRemoteSupportComponent.h"

APMRemoteSupportComponent::APMRemoteSupportComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Remote Support"))
{
}

QString APMRemoteSupportComponent::name(void) const
{
    return _name;
}

QString APMRemoteSupportComponent::description(void) const
{
    return tr("On this menu you can forward mavlink telemetry to an ardupilot support engineer.");
}

QString APMRemoteSupportComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/ForwardingSupportIcon.svg");
}

bool APMRemoteSupportComponent::requiresSetup(void) const
{
    return false;
}

bool APMRemoteSupportComponent::setupComplete(void) const
{
    return true;
}

QStringList APMRemoteSupportComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMRemoteSupportComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMRemoteSupportComponent.qml"));

}

QUrl APMRemoteSupportComponent::summaryQmlSource(void) const
{
    return QUrl();
}
