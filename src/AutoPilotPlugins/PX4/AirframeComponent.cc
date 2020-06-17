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

#include "AirframeComponent.h"
#include "ParameterManager.h"

AirframeComponent::AirframeComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Airframe"))
{

}

QString AirframeComponent::name(void) const
{
    return _name;
}

QString AirframeComponent::description(void) const
{
    return tr("Airframe Setup is used to select the airframe that matches your vehicle. "
              "This will in turn set up the various tuning values for flight parameters.");
}

QString AirframeComponent::iconResource(void) const
{
    return "/qmlimages/AirframeComponentIcon.png";
}

bool AirframeComponent::requiresSetup(void) const
{
    return true;
}

bool AirframeComponent::setupComplete(void) const
{
    return _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, QStringLiteral("SYS_AUTOSTART"))->rawValue().toInt() != 0;
}

QStringList AirframeComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList(QStringLiteral("SYS_AUTOSTART"));
}

QUrl AirframeComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/AirframeComponent.qml");
}

QUrl AirframeComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/AirframeComponentSummary.qml");
}
