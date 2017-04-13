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

#include "GimbalComponent.h"
#include "PX4AutoPilotPlugin.h"

GimbalComponent::GimbalComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Gimbal"))
{
}

QString
GimbalComponent::name(void) const
{
    return _name;
}

QString
GimbalComponent::description(void) const
{
    return tr("Gimbal Calibration.");
}

QString
GimbalComponent::iconResource(void) const
{
    return "/typhoonh/img/CogWheel.svg";
}

bool
GimbalComponent::requiresSetup(void) const
{
    return false;
}

bool
GimbalComponent::setupComplete(void) const
{
    return true;
}

QStringList
GimbalComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;
    return list;
}

QUrl
GimbalComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/typhoonh/GimbalComponent.qml");
}

QUrl
GimbalComponent::summaryQmlSource(void) const
{
    return QUrl();
}
