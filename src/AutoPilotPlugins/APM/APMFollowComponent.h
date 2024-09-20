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

class APMFollowComponent : public VehicleComponent
{
    Q_OBJECT

public:
    APMFollowComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);
    ~APMFollowComponent();

    QString name() const final { return _name; }
    QString description() const final { return QStringLiteral("Follow Me Setup is used to configure support for the vehicle following the ground station location."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/FollowComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMFollowComponent.qml")); }
    QUrl summaryQmlSource() const final { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMFollowComponentSummary.qml")); }
    bool allowSetupWhileArmed() const final { return true; }
    bool allowSetupWhileFlying() const final { return true; }

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

private:
    const QString _name;
};
