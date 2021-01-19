/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef OutputComponent_H
#define OutputComponent_H

#include "VehicleComponent.h"

/// @file
///     @brief Output configuration
///     @author Gus Grubba <gus@auterion.com>

class OutputComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    OutputComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
    // Overrides from VehicleComponent
    QStringList setupCompleteChangedTriggerList() const override;
    
    // Overrides from VehicleComponent
    QString name                    () const override;
    QString description             () const override;
    QString iconResource            () const override;
    bool    requiresSetup           () const override;
    bool    setupComplete           () const override;
    QUrl    setupSource             () const override;
    QUrl    summaryQmlSource        () const override;
    bool    allowSetupWhileArmed    () const override { return true; }

private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
