#include "PX4EscComponent.h"

PX4EscComponent::PX4EscComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("ESC"))
{}

QString PX4EscComponent::description() const
{
    return tr("Calibrate PWM ESCs and configure DShot and DroneCAN ESC settings.");
}

QString PX4EscComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/bolt.svg");
}

QStringList PX4EscComponent::searchKeywords() const
{
    return {
        tr("Electronic speed controller"), tr("Calibration"), tr("Motor"), tr("PWM"), tr("DShot"), tr("DroneCAN"),
    };
}

QUrl PX4EscComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4EscComponent.qml"));
}
