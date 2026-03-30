#pragma once

#include "VehicleComponent.h"

class APMAdvancedTuningCopterComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMAdvancedTuningCopterComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Advanced rate controller PID tuning with live telemetry charts."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/TuningComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final { return QUrl(); }
    bool allowSetupWhileArmed() const final { return true; }

private:
    const QString _name = tr("Tuning - Advanced");
};
