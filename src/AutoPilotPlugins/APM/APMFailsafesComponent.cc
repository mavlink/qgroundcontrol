#include "APMFailsafesComponent.h"
#include "Vehicle.h"
#include "QGCMAVLink.h"

APMFailsafesComponent::APMFailsafesComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}

QString APMFailsafesComponent::vehicleConfigJson() const
{
    return QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/APM/VehicleConfig/APMFailsafes.VehicleConfig.json");
}

QString APMFailsafesComponent::description() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_SUBMARINE:
        return tr("Configure failsafe actions and leak detection.");
    case MAV_TYPE_GROUND_ROVER:
        return tr("Configure battery, GCS, throttle, and EKF failsafes.");
    case MAV_TYPE_FIXED_WING:
        return tr("Configure battery, GCS, and throttle failsafes.");
    default:
        return tr("Configure battery, GCS, RC, throttle, EKF, and dead reckoning failsafes.");
    }
}

QUrl APMFailsafesComponent::setupSource() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_SUBMARINE:
    case MAV_TYPE_FIXED_WING:
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_GROUND_ROVER:
        // Generated from APMFailsafes.VehicleConfig.json
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFailsafesComponent.qml"));
    default:
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMNotSupported.qml"));
    }
}

QUrl APMFailsafesComponent::summaryQmlSource() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_SUBMARINE:
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFailsafesComponentSummarySub.qml"));
    case MAV_TYPE_FIXED_WING:
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_GROUND_ROVER:
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFailsafesComponentSummary.qml"));
    default:
        return QUrl();
    }
}
