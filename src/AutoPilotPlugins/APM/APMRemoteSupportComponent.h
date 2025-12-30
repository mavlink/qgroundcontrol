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

class APMRemoteSupportComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMRemoteSupportComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("On this menu you can forward mavlink telemetry to an ardupilot support engineer."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/ForwardingSupportIcon.svg"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMRemoteSupportComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl(); }

private:
    const QString _name = tr("Remote Support");
};
