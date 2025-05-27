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

class APMAirframeComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMAirframeComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final;

    QString name() const final { return _name; }
    QString description() const final { return tr("Frame Setup is used to select the airframe which matches your vehicle."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/AirframeComponentIcon.png"); }
    bool requiresSetup() const final { return _requiresFrameSetup; }
    bool setupComplete() const final;
    QUrl setupSource() const final { return (_requiresFrameSetup ? QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMAirframeComponent.qml")) : QUrl()); }
    QUrl summaryQmlSource() const final { return (_requiresFrameSetup ? QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMAirframeComponentSummary.qml")) : QUrl()); }

private:
    bool _requiresFrameSetup = false; ///< true: FRAME parameter must be set
    const QString _name = tr("Frame");
    Fact *_frameClassFact = nullptr;

    static constexpr const char *_frameClassParam = "FRAME_CLASS";
};
