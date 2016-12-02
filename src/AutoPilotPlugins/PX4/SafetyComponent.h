/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef SafetyComponent_H
#define SafetyComponent_H

#include "VehicleComponent.h"

/// @file
///     @brief The Radio VehicleComponent is used to calibrate the trasmitter and assign function mapping
///             to channels.
///     @author Don Gagne <don@thegagnes.com>

class SafetyComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    SafetyComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList(void) const;
    
    // Virtuals from VehicleComponent
    QString name(void) const final;
    QString description(void) const final;
    QString iconResource(void) const final;
    bool requiresSetup(void) const final;
    bool setupComplete(void) const final;
    QUrl setupSource(void) const final;
    QUrl summaryQmlSource(void) const final;
    QString prerequisiteSetup(void) const final;
    bool allowSetupWhileArmed(void) const final { return true; }

private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
