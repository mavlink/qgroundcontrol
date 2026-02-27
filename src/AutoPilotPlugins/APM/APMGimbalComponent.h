#pragma once

#include "VehicleComponent.h"

class APMGimbalComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMGimbalComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Gimbal Setup"); }
    QString iconResource() const final { return QStringLiteral("/res/CameraGimbal.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final { return QUrl(); }

private:
    const QString _name = tr("Gimbal");
};
