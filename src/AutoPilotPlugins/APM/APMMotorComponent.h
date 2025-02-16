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

class AutoPilotPlugin;

class APMMotorComponent : public MotorComponent
{
    Q_OBJECT

public:
    APMMotorComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    // VehicleComponent overrides
    QUrl setupSource            (void) const override;
    bool allowSetupWhileArmed   (void) const override { return true; }

private:
    const QString   _name;
};
