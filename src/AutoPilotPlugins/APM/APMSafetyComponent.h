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

class APMSafetyComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMSafetyComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final;
    QString iconResource() const final { return QStringLiteral("/qmlimages/SafetyComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; } // FIXME: What aboout invalid settings?
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final;
    bool allowSetupWhileArmed() const final { return true; }
    bool allowSetupWhileFlying() const final { return true; }

private:
    const QString _name = tr("Safety");
};
