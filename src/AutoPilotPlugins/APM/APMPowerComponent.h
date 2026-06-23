#pragma once

#include "VehicleComponent.h"

class APMPowerComponent : public VehicleComponent
{
    Q_OBJECT

public:
    APMPowerComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Configure battery monitoring and capacity parameters."); }
    QString vehicleConfigJson() const final;
    QString iconResource() const final { return QStringLiteral("/qmlimages/Battery.svg"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMPowerComponentSummary.qml")); }
    bool allowSetupWhileArmed() const final { return true; }

private:
    const QString _name = tr("Power");
};
