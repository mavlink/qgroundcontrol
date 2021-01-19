/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Gus Grubba <gus@auterion.com>

#include "OutputComponent.h"
#include "PX4AutoPilotPlugin.h"

OutputComponent::OutputComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Output"))
{
}

QString OutputComponent::name() const
{
    return _name;
}

QString OutputComponent::description() const
{
    return QString();
}

QString OutputComponent::iconResource() const
{
    return "/qmlimages/OutputComponentIcon.svg";
}

bool OutputComponent::requiresSetup() const
{
    return true;
}

bool OutputComponent::setupComplete() const
{
    return true;
}

QStringList OutputComponent::setupCompleteChangedTriggerList() const
{
    return QStringList();
}

QUrl OutputComponent::setupSource() const
{
    return QUrl::fromUserInput("qrc:/qml/OutputComponent.qml");
}

QUrl OutputComponent::summaryQmlSource() const
{
    return QUrl::fromUserInput("qrc:/qml/OutputComponentSummary.qml");
}
