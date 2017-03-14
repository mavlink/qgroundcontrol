/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "SafetyComponent.h"
#include "PX4AutoPilotPlugin.h"

SafetyComponent::SafetyComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Safety"))
{
}

QString SafetyComponent::name(void) const
{
    return _name;
}

QString SafetyComponent::description(void) const
{
    return tr("Safety Setup is used to setup triggers for Return to Land as well as the settings for Return to Land itself.");
}

QString SafetyComponent::iconResource(void) const
{
    return "/qmlimages/SafetyComponentIcon.png";
}

bool SafetyComponent::requiresSetup(void) const
{
    return false;
}

bool SafetyComponent::setupComplete(void) const
{
    // FIXME: What aboout invalid settings?
    return true;
}

QStringList SafetyComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl SafetyComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/SafetyComponent.qml");
}

QUrl SafetyComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/SafetyComponentSummary.qml");
}
