/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMSensorsComponent_H
#define APMSensorsComponent_H

#include "VehicleComponent.h"

class APMSensorsComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    APMSensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    bool compassSetupNeeded(void) const;
    bool accelSetupNeeded(void) const;

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
    
private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
