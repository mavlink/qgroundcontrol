#pragma once

#include "VehicleComponent.h"

class APMSensorsComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMSensorsComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    bool compassSetupNeeded() const;
    bool accelSetupNeeded() const;

    QStringList setupCompleteChangedTriggerList() const final;

    QString name() const final { return _name; }
    QString description() const final { return tr("Configure and calibrate compass, accelerometer, and other onboard sensors."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/SensorsComponentIcon.png"); }
    bool requiresSetup() const final { return true; }
    bool setupComplete() const final { return (!compassSetupNeeded() && !accelSetupNeeded()); }
    QUrl setupSource() const final { return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSensorsComponent.qml"); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSensorsComponentSummary.qml"); }

private:
    const QString _name = tr("Sensors");
};
