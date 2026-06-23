#pragma once

#include "VehicleComponent.h"

class APMESCComponent : public VehicleComponent
{
    Q_OBJECT

public:
    APMESCComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Configure and calibrate Electronic Speed Controllers."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/EscIndicator.svg"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMESCComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMESCComponentSummary.qml")); }
    bool allowSetupWhileArmed() const final { return true; }

private:
    const QString _name = tr("ESC");
};
