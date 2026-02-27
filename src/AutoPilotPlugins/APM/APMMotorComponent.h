#pragma once

#include "MotorComponent.h"

class APMMotorComponent : public MotorComponent
{
    Q_OBJECT

public:
    explicit APMMotorComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QUrl setupSource() const final;
    bool allowSetupWhileArmed() const final { return true; }

private:
    const QString _name = tr("Motors");
};
