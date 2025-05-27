/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMMotorComponent.h"
#include "Vehicle.h"

APMMotorComponent::APMMotorComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : MotorComponent(vehicle, autopilot, parent)
{

}

QUrl APMMotorComponent::setupSource() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_SUBMARINE:
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSubMotorComponent.qml"));
    default:
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMMotorComponent.qml"));
    }
}
