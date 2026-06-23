#pragma once

#include "VehicleComponent.h"

class APMLoggingComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMLoggingComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Configure ArduPilot logging parameters."); }
    QString vehicleConfigJson() const final;
    QString iconResource() const final { return QStringLiteral("/qmlimages/OnboardLogIcon.svg"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final { return QUrl(); }
    bool allowSetupWhileArmed() const final { return true; }
    bool allowSetupWhileFlying() const final { return true; }

private:
    const QString _name = tr("Logging");
};
