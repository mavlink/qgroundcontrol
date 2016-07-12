/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MotorComponent_H
#define MotorComponent_H

#include "VehicleComponent.h"
#include "Fact.h"

class MotorComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    MotorComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList(void) const final;
    
    // Virtuals from VehicleComponent
    QString name(void) const final;
    QString description(void) const final;
    QString iconResource(void) const final;
    bool requiresSetup(void) const final;
    bool setupComplete(void) const final;
    QUrl setupSource(void) const final;
    QUrl summaryQmlSource(void) const final;
    QString prerequisiteSetup(void) const final;

private:
    const QString   _name;
};

#endif
