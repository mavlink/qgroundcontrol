/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMHeliComponent.h"
#include "APMAutoPilotPlugin.h"

APMHeliComponent::APMHeliComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("Heli"))
{
}

QString APMHeliComponent::name(void) const
{
    return _name;
}

QString APMHeliComponent::description(void) const
{
    return tr("Heli Setup is used to setup parameters which are specific to a helicopter.");
}

QString APMHeliComponent::iconResource(void) const
{
    return "/res/helicoptericon.svg";
}

bool APMHeliComponent::requiresSetup(void) const
{
    return false;
}

bool APMHeliComponent::setupComplete(void) const
{
    return true;
}

QStringList APMHeliComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMHeliComponent::setupSource(void) const
{
    return QStringLiteral("qrc:/qml/APMHeliComponent.qml");
}

QUrl APMHeliComponent::summaryQmlSource(void) const
{
    return QUrl();
}
