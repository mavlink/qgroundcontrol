/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef SyslinkComponent_H
#define SyslinkComponent_H

#include "VehicleComponent.h"

class SyslinkComponent : public VehicleComponent
{
    Q_OBJECT
public:
    SyslinkComponent            (Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList() const;
    
    // Virtuals from VehicleComponent
    QString name                () const;
    QString description         () const;
    QString iconResource        () const;
    bool    requiresSetup       () const;
    bool    setupComplete       () const;
    QUrl    setupSource         () const;
    QUrl    summaryQmlSource    () const;
    QString prerequisiteSetup   () const;
    
private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
