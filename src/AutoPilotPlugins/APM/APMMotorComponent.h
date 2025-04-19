/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MotorComponent.h"

class APMMotorComponent : public MotorComponent
{
    Q_OBJECT

public:
    explicit APMMotorComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);

    QUrl setupSource() const final;
    bool allowSetupWhileArmed() const final { return true; }

private:
    const QString _name = tr("Motors");
};
