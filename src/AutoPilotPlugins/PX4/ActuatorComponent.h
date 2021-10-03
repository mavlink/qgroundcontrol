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
#include "Fact.h"

#include "Actuators/Actuators.h"

class ActuatorComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    ActuatorComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList(void) const final;
    
    // Virtuals from VehicleComponent
    QString name(void) const final;
    QString description(void) const final;
    QString iconResource(void) const final;
    bool requiresSetup(void) const final;
    bool setupComplete(void) const final;
    virtual QUrl setupSource(void) const;
    QUrl summaryQmlSource(void) const final;

private:
    const QString   _name;
    Actuators& _actuators;
};

