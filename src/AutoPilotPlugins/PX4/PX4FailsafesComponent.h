#pragma once

#include "VehicleComponent.h"

class PX4FailsafesComponent : public VehicleComponent
{
    Q_OBJECT

public:
    PX4FailsafesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }

    QString description() const final;
    QString iconResource() const final;

    bool requiresSetup() const final { return false; }

    bool setupComplete() const final { return true; }

    QUrl setupSource() const final;

    QUrl summaryQmlSource() const final { return QUrl(); }

    bool allowSetupWhileArmed() const final { return true; }

    bool allowSetupWhileFlying() const final { return true; }

private:
    const QString _name;
};
