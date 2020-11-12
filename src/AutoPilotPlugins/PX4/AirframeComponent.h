/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef AIRFRAMECOMPONENT_H
#define AIRFRAMECOMPONENT_H

#include "VehicleComponent.h"

/// @file
///     @brief The Airframe VehicleComponent is used to set the SYS_AUTOSTART airframe id.
///     @author Don Gagne <don@thegagnes.com>

class AirframeComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    AirframeComponent(Vehicle* vehicles, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
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

private:
    const QString   _name;
    QVariantList     _summaryItems;
};

#endif
