#pragma once

#include "VehicleComponent.h"

class Fact;
class ParameterManager;

class PX4SerialPortsComponent : public VehicleComponent
{
    Q_OBJECT

public:
    PX4SerialPortsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    /// Parameters which select the serial port a function runs on. The page is only shown when there are some.
    static QList<Fact*> functionFacts(ParameterManager* parameterManager);

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

private:
    const QString _name;
};
