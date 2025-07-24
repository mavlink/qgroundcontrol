/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "VehicleComponent.h"

class APMHeliComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMHeliComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Heli Setup is used to setup parameters which are specific to a helicopter."); }
    QString iconResource() const final { return QStringLiteral("/res/helicoptericon.svg"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMHeliComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl(); }
    bool allowSetupWhileArmed() const final { return true; }

private:
    const QString _name = tr("Heli");
};
