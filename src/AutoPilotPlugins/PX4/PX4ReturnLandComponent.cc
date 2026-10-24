#include "PX4ReturnLandComponent.h"

PX4ReturnLandComponent::PX4ReturnLandComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Return & Land"))
{}

QString PX4ReturnLandComponent::description() const
{
    return tr("Configure where and how high the vehicle returns, what it does on arrival, and how it lands.");
}

QString PX4ReturnLandComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/home.svg");
}

QStringList PX4ReturnLandComponent::searchKeywords() const
{
    return {
        tr("RTL"),     tr("Return to launch"), tr("Return home"), tr("Loiter"),   tr("Landing"),
        tr("Descent"), tr("Disarm"),           tr("Touchdown"),   tr("Altitude"),
    };
}

QUrl PX4ReturnLandComponent::setupSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4ReturnLandComponent.qml"));
}

QUrl PX4ReturnLandComponent::summaryQmlSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4ReturnLandComponentSummary.qml"));
}
