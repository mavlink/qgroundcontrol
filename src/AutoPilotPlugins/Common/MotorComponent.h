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

class MotorComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit MotorComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const override { return QStringList(); }
    QString name() const override { return _name; }
    QString description() const override { return tr("Motors Setup is used to manually test motor control and direction."); }
    QString iconResource() const override { return QStringLiteral("/qmlimages/MotorComponentIcon.svg"); }
    bool requiresSetup() const override { return false; }
    bool setupComplete() const override { return true; }
    QUrl setupSource() const override { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/Controls/MotorComponent.qml")); }
    QUrl summaryQmlSource() const override { return QUrl(); }

private:
    const QString _name;
};
