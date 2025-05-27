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

class Fact;

class APMRadioComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMRadioComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("The Radio Component is used to setup which channels on your RC Transmitter you will use for each vehicle control such as Roll, Pitch, Yaw and Throttle. "
                                                  "It also allows you to assign switches and dials to the various flight modes. "
                                                  "Prior to flight you must also calibrate the extents for all of your channels."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/RadioComponentIcon.png"); }
    bool requiresSetup() const final { return true; }
    bool setupComplete() const final;
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/Common/RadioComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMRadioComponentSummary.qml")); }

private slots:
    void _triggerChanged();

private:
    void _connectSetupTriggers();

    const QString _name = tr("Radio");
    const QStringList _mapParams = {
        QStringLiteral("RCMAP_ROLL"),
        QStringLiteral("RCMAP_PITCH"),
        QStringLiteral("RCMAP_YAW"),
        QStringLiteral("RCMAP_THROTTLE")
    };
    QList<Fact*> _triggerFacts;
};
