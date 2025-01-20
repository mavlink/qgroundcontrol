/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMMotorComponent.h"
#include "AutoPilotPlugin.h"
#include "Vehicle.h"

APMMotorComponent::APMMotorComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    MotorComponent(vehicle, autopilot, parent),
    _name(tr("Motors"))
{

}

QUrl APMMotorComponent::setupSource(void) const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_SUBMARINE:
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMSubMotorComponent.qml"));
    default:
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMMotorComponent.qml"));
    }
}
