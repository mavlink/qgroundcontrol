/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef PowerComponent_H
#define PowerComponent_H

#include "VehicleComponent.h"

/// @file
///     @brief Battery, propeller and magnetometer settings
///     @author Gus Grubba <mavlink@grubba.com>

class PowerComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    PowerComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    // Virtuals from VehicleComponent
    virtual QStringList setupCompleteChangedTriggerList(void) const;
    
    // Virtuals from VehicleComponent
    virtual QString     name                    (void) const;
    virtual QString     description             (void) const;
    virtual QString     iconResource            (void) const;
    virtual bool        requiresSetup           (void) const;
    virtual bool        setupComplete           (void) const;
    virtual QUrl        setupSource             (void) const;
    virtual QUrl        summaryQmlSource        (void) const;
    
private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
