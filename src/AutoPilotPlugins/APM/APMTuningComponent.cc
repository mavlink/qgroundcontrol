/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMTuningComponent.h"
#include "Vehicle.h"

APMTuningComponent::APMTuningComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}

QUrl APMTuningComponent::setupSource() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
        return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMTuningComponentCopter.qml");
    case MAV_TYPE_SUBMARINE:
        return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMTuningComponentSub.qml");
    default:
        return QUrl::fromUserInput(QString());
    }
}
