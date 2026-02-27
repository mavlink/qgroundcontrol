#pragma once

#include "VehicleComponent.h"

class ScriptingComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit ScriptingComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const override { return QStringList(); }
    QString name() const override { return _name; }
    QString description() const override { return tr("Provides access to onboard script management."); }
    QString iconResource() const override { return QStringLiteral("/InstrumentValueIcons/folder-outline.svg"); }
    bool requiresSetup() const override { return false; }
    bool setupComplete() const override { return true; }
    QUrl setupSource() const override { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/Common/ScriptingComponent.qml")); }
    QUrl summaryQmlSource() const override { return QUrl(); }

private:
    const QString _name;
};
