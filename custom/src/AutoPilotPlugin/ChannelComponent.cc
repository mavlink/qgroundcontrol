/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
///     @brief  Channel Calibration
///     @author Gus Grubba <mavlink@grubba.com>

#include "ChannelComponent.h"
#include "PX4AutoPilotPlugin.h"

ChannelComponent::ChannelComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Channels"))
{
}

QString
ChannelComponent::name(void) const
{
    return _name;
}

QString
ChannelComponent::description(void) const
{
    return tr("Raw Channel Calibration.");
}

QString
ChannelComponent::iconResource(void) const
{
    return "/typhoonh/img/CogWheel.svg";
}

bool
ChannelComponent::requiresSetup(void) const
{
    return false;
}

bool
ChannelComponent::setupComplete(void) const
{
    return true;
}

QStringList
ChannelComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;
    return list;
}

QUrl
ChannelComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/typhoonh/ChannelComponent.qml");
}

QUrl
ChannelComponent::summaryQmlSource(void) const
{
    return QUrl();
}
