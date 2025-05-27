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

class APMSensorsComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMSensorsComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    bool compassSetupNeeded() const;
    bool accelSetupNeeded() const;

    QStringList setupCompleteChangedTriggerList() const final;

    QString name() const final { return _name; }
    QString description() const final { return tr("Sensors Setup is used to calibrate the sensors within your vehicle."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/SensorsComponentIcon.png"); }
    bool requiresSetup() const final { return true; }
    bool setupComplete() const final { return (!compassSetupNeeded() && !accelSetupNeeded()); }
    QUrl setupSource() const final { return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSensorsComponent.qml"); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSensorsComponentSummary.qml"); }

private:
    const QString _name = tr("Sensors");
};
