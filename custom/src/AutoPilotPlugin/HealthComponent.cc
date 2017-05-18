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

#include "QGCApplication.h"
#include "Vehicle.h"

#include "HealthComponent.h"
#include "PX4AutoPilotPlugin.h"

HealthComponent::HealthComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Sensors Health"))
{
}

QString
HealthComponent::name(void) const
{
    return _name;
}

QString
HealthComponent::description(void) const
{
    return _name;
}

QString
HealthComponent::iconResource(void) const
{
    return "/typhoonh/img/CogWheel.svg";
}

bool
HealthComponent::requiresSetup(void) const
{
    return false;
}

bool
HealthComponent::setupComplete(void) const
{
    //-- Ideally we would show a red dot if something is amiss but this causes an error
    //   message to pop up.
#if 0
    bool res = true;
    if(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        res = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->unhealthySensors().size() > 0;
    }
    return res;
#else
    return true;
#endif
}

QStringList
HealthComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;
    return list;
}

QUrl
HealthComponent::setupSource(void) const
{
    return QUrl();
}

QUrl
HealthComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/typhoonh/HealthComponentSummary.qml");
}
