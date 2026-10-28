#include "PX4CollisionComponent.h"

#include "Fact.h"

PX4CollisionComponent::PX4CollisionComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Collision"))
{}

QString PX4CollisionComponent::description() const
{
    return tr("Configure collision prevention, which keeps the vehicle away from obstacles seen by distance sensors.");
}

QString PX4CollisionComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/radar.svg");
}

QStringList PX4CollisionComponent::searchKeywords() const
{
    return {
        tr("Collision prevention"), tr("Avoidance"), tr("Obstacle"), tr("Proximity"), tr("Distance sensor"),
    };
}

QUrl PX4CollisionComponent::setupSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4CollisionComponent.qml"));
}

QList<VehicleSummaryItem> PX4CollisionComponent::summaryItems() const
{
    const Fact* const distance = _parameterFact(QStringLiteral("CP_DIST"));
    if (!distance) {
        return {};
    }

    if (distance->rawValue().toFloat() > 0) {
        return {{tr("Collision Prevention"), _valueWithUnits(distance), VehicleSummaryItem::Normal}};
    }

    return {{tr("Collision Prevention"), tr("Disabled"), VehicleSummaryItem::Muted}};
}
