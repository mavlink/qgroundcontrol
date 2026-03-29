#pragma once

#include "VehicleComponent.h"

class APMFlightSafetyComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMFlightSafetyComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final;
    QString vehicleConfigJson() const final;
    QString iconResource() const final { return QStringLiteral("/qmlimages/SafetyComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; } // FIXME: What about invalid settings?
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final;
    bool allowSetupWhileArmed() const final { return true; }
    bool allowSetupWhileFlying() const final { return true; }

private:
    const QString _name = tr("Flight Safety");
};
