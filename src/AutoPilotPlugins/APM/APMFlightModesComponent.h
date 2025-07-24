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

class APMFlightModesComponent : public VehicleComponent
{
    Q_OBJECT

public:
    APMFlightModesComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Flight Modes Setup is used to configure the transmitter switches associated with Flight Modes."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/FlightModesComponentIcon.png"); }
    bool requiresSetup() const final { return true; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFlightModesComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMFlightModesComponentSummary.qml")); }

private:
    const QString _name = tr("Flight Modes");
};
