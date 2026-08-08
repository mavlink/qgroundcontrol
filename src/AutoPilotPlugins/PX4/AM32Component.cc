/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AM32Component.h"
#include "Vehicle.h"

AM32Component::AM32Component(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("AM32 ESC"))
{
}

QString AM32Component::name(void) const
{
    return _name;
}

QString AM32Component::description(void) const
{
    return tr("AM32 ESC Setup is used to configure AM32 ESC settings.");
}

QString AM32Component::iconResource(void) const
{
    return "/qmlimages/EscIndicator.svg";
}

bool AM32Component::requiresSetup(void) const
{
    return false;
}

bool AM32Component::setupComplete(void) const
{
    return true;
}

QStringList AM32Component::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl AM32Component::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/AM32Component.qml");
}

QUrl AM32Component::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/AM32ComponentSummary.qml");
}
