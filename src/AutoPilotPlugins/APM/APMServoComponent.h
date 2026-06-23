#pragma once

#include "VehicleComponent.h"

class APMServoComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMServoComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    // VehicleComponent overrides
    QString     name() const final                 { return _name; }
    QString     description() const final          { return tr("Configure servo PWM limits, trim, direction, and function assignment."); }
    QString     iconResource() const final         { return QStringLiteral("/qmlimages/MotorComponentIcon.svg"); }
    bool        requiresSetup() const final        { return false; }
    bool        setupComplete() const final        { return true; }
    QUrl        setupSource() const final;
    QUrl        summaryQmlSource() const final     { return QUrl(); }
    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

private:
    const QString _name = tr("Servo Outputs");
};
