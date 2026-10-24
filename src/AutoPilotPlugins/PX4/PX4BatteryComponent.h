#pragma once

#include "VehicleComponent.h"

class PX4BatteryComponent : public VehicleComponent
{
    Q_OBJECT

public:
    PX4BatteryComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList() const final;

    QString name() const final { return _name; }

    QString description() const final;
    QString iconResource() const final;
    QStringList searchKeywords() const final;

    bool requiresSetup() const final { return true; }

    bool setupComplete() const final;

    QUrl setupSource() const final;

    QUrl summaryQmlSource() const final;

    bool allowSetupWhileArmed() const final { return true; }

private:
    const QString _name;
};
