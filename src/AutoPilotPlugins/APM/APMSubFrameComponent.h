#pragma once

#include "VehicleComponent.h"

class APMSubFrameComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMSubFrameComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Configure the submarine motor layout and load default parameters."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/SubFrameComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSubFrameComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSubFrameComponentSummary.qml")); }

private:
    const QString _name = tr("Frame");
};
