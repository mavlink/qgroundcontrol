#pragma once

#include "VehicleComponent.h"

class FlightModesComponent : public VehicleComponent
{
    Q_OBJECT

public:
    FlightModesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    // Overrides from VehicleComponent
    QString name(void) const final;
    QString description(void) const final;
    QString iconResource(void) const final;
    QUrl setupSource(void) const final;
    QUrl summaryQmlSource(void) const final;
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

private:
    const QString   _name;
    QVariantList    _summaryItems;
};
