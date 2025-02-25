/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMFollowComponent.h"
#include "APMAutoPilotPlugin.h"

APMFollowComponent::APMFollowComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(QStringLiteral("Follow Me"))
{
    // qCDebug() << Q_FUNC_INFO << this;
}

APMFollowComponent::~APMFollowComponent()
{
    // qCDebug() << Q_FUNC_INFO << this;
}
