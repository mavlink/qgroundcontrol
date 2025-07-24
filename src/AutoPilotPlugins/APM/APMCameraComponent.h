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

class APMCameraComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit APMCameraComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }

    QString name() const final { return _name; }
    QString description() const final { return tr("Camera setup is used to adjust camera and gimbal settings."); }
    QString iconResource() const final { return QStringLiteral("/qmlimages/CameraComponentIcon.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final { return true; }
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final { return QUrl(); }

private:
    const QString _name = tr("Camera");
};
