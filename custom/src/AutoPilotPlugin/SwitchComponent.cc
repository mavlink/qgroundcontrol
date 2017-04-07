/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
///     @brief  Switch assignment
///     @author Gus Grubba <mavlink@grubba.com>

#include "SwitchComponent.h"
#include "PX4AutoPilotPlugin.h"

SwitchComponent::SwitchComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Switches"))
{
}

QString
SwitchComponent::name(void) const
{
    return _name;
}

QString
SwitchComponent::description(void) const
{
    return tr("ST16 Custom Switch Assignment.");
}

QString
SwitchComponent::iconResource(void) const
{
    return "/typhoonh/img/switch.svg";
}

bool
SwitchComponent::requiresSetup(void) const
{
    return false;
}

bool
SwitchComponent::setupComplete(void) const
{
    return true;
}

QStringList
SwitchComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;
    list << QStringLiteral("COM_FLTMODE6");
    return list;
}

QUrl
SwitchComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/typhoonh/SwitchComponent.qml");
}

QUrl
SwitchComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/typhoonh/SwitchComponentSummary.qml");
}
