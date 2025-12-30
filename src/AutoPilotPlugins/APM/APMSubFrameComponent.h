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

class APMSubFrameComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMSubFrameComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Frame setup allows you to choose your vehicle's motor configuration. Install <b>clockwise</b>" \
                                                  "<br>propellers on the <b>green thrusters</b> and <b>counter-clockwise</b> propellers on the <b>blue thrusters</b>" \
                                                  "<br>(or vice-versa). The flight controller will need to be rebooted to apply changes." \
                                                  "<br>When selecting a frame, you can choose to load the default parameter set for that frame configuration if available."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/SubFrameComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSubFrameComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMSubFrameComponentSummary.qml")); }

private:
    const QString _name = tr("Frame");
};
