#pragma once

#include "VehicleComponent.h"

class APMAirspeedComponent : public VehicleComponent
{
    Q_OBJECT

public:
    APMAirspeedComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Configure airspeed sensor type and calibration."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/SensorsComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMAirspeedComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMAirspeedComponentSummary.qml")); }
    bool allowSetupWhileArmed() const final { return true; }

private:
    const QString _name = tr("Airspeed");
};
