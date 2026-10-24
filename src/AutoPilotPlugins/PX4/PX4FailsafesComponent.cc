#include "PX4FailsafesComponent.h"

PX4FailsafesComponent::PX4FailsafesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Failsafes"))
{}

QString PX4FailsafesComponent::description() const
{
    return tr("Configure what the vehicle does when it loses a link, runs low on power or detects a fault.");
}

QString PX4FailsafesComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/exclamation-outline.svg");
}

QStringList PX4FailsafesComponent::searchKeywords() const
{
    return {
        tr("RC loss"),           tr("Joystick loss"), tr("Radio"),          tr("Signal loss"),
        tr("Data link loss"),    tr("Telemetry"),     tr("Ground station"), tr("GCS"),
        tr("Geofence"),          tr("Fence"),         tr("Breach"),         tr("Low battery"),
        tr("Critical battery"),  tr("GPS loss"),      tr("Wind"),           tr("Flight time"),
        tr("Traffic avoidance"),
    };
}

QUrl PX4FailsafesComponent::setupSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4FailsafesComponent.qml"));
}
