#pragma once

#include "VehicleComponent.h"

class APMFollowComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMFollowComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QString name() const final { return _name; }
    QString description() const final { return tr("Configure the vehicle to track the ground station position."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/FollowComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFollowComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFollowComponentSummary.qml")); }
    bool allowSetupWhileArmed() const final { return true; }
    bool allowSetupWhileFlying() const final { return true; }

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

private:
    const QString _name = tr("Follow Me");
};
