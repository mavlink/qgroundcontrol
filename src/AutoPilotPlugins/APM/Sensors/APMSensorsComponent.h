#pragma once

#include "SensorsComponentBase.h"

class APMSensorsComponent : public SensorsComponentBase
{
    Q_OBJECT

public:
    explicit APMSensorsComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    bool compassSetupNeeded() const;
    bool accelSetupNeeded() const;

    QStringList setupCompleteChangedTriggerList() const final;

    bool setupComplete() const final { return (!compassSetupNeeded() && !accelSetupNeeded()); }
    QUrl setupSource() const final { return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/Sensors/APMSensorsComponent.qml"); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/Sensors/APMSensorsComponentSummary.qml"); }
};
