/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    SafetyComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList(void) const override;
    
    // Virtuals from VehicleComponent
    QString name                (void) const override;
    QString description         (void) const override;
    QString iconResource        (void) const override;
    bool requiresSetup          (void) const override;
    bool setupComplete          (void) const override;
    QUrl setupSource            (void) const override;
    QUrl summaryQmlSource       (void) const override;
    bool allowSetupWhileArmed   (void) const override { return true; }
    bool allowSetupWhileFlying  (void) const override { return true; }

private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
