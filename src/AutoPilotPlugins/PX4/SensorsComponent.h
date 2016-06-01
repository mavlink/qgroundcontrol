/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef SENSORSCOMPONENT_H
#define SENSORSCOMPONENT_H

#include "VehicleComponent.h"

/// @file
///     @brief The Sensors VehicleComponent is used to calibrate the the various sensors associated with the board.
///     @author Don Gagne <don@thegagnes.com>

class SensorsComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    // Virtuals from VehicleComponent
    virtual QStringList setupCompleteChangedTriggerList(void) const;
    
    // Virtuals from VehicleComponent
    virtual QString name(void) const;
    virtual QString description(void) const;
    virtual QString iconResource(void) const;
    virtual bool requiresSetup(void) const;
    virtual bool setupComplete(void) const;
    virtual QUrl setupSource(void) const;
    virtual QUrl summaryQmlSource(void) const;
    virtual QString prerequisiteSetup(void) const;
    
private:
    const QString   _name;
    QVariantList    _summaryItems;

    static const char* _airspeedBreaker;
};

#endif
