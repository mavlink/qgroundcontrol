/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMMotorComponent_H
#define APMMotorComponent_H

#include "MotorComponent.h"

class APMMotorComponent : public MotorComponent
{
    Q_OBJECT

public:
    APMMotorComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    // VehicleComponent overrides
    QUrl setupSource            (void) const override;
    bool allowSetupWhileArmed   (void) const override { return true; }

    Q_INVOKABLE QString motorIndexToLetter(int index);

private:
    const QString   _name;
};

#endif
