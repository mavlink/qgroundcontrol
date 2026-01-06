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

class Joystick;
class JoystickManager;

class JoystickComponent : public VehicleComponent
{
    Q_OBJECT

public:
    JoystickComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }
    QString name() const final { return _name; }
    QString description() const final;
    QString iconResource() const final { return QStringLiteral("/qmlimages/Joystick.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final;
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final;

private slots:
    void _activeJoystickChanged(Joystick *joystick);

private:
    const QString _name;
    Joystick *_activeJoystick = nullptr;
};
