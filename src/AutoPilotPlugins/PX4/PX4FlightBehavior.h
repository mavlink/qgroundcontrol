/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "VehicleComponent.h"

class PX4FlightBehavior : public VehicleComponent
{
    Q_OBJECT
    
public:
    PX4FlightBehavior(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList() const final;
    
    // Virtuals from VehicleComponent
    QString name() const final;
    QString description() const final;
    QString iconResource() const final;
    bool requiresSetup() const final;
    bool setupComplete() const final;
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final;
    bool allowSetupWhileArmed() const final { return true; }
    bool allowSetupWhileFlying() const final { return true; }

private:
    const QString   _name;
};
