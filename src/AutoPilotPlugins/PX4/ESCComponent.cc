/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ESCComponent.h"
#include "Vehicle.h"

ESCComponent::ESCComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("ESC"))
{
}

QString ESCComponent::name(void) const
{
    return _name;
}

QString ESCComponent::description(void) const
{
    return tr("ESC Setup is used to configure ESC settings.");
}

QString ESCComponent::iconResource(void) const
{
    return "/qmlimages/EscIndicator.svg";
}

bool ESCComponent::requiresSetup(void) const
{
    return false;
}

bool ESCComponent::setupComplete(void) const
{
    return true;
}

QStringList ESCComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl ESCComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/ESCComponent.qml");
}

QUrl ESCComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/ESCComponentSummary.qml");
}