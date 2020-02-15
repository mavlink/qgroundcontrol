/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMPowerComponent_H
#define APMPowerComponent_H

#include "VehicleComponent.h"

class APMPowerComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    APMPowerComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
    // Overrides from VehicleComponent
    QStringList setupCompleteChangedTriggerList(void) const override { return QStringList(); }
    
    // Virtuals from VehicleComponent
    QString name                    (void) const override;
    QString description             (void) const override;
    QString iconResource            (void) const override;
    bool    requiresSetup           (void) const override { return false; }
    bool    setupComplete           (void) const override { return true; }
    QUrl    setupSource             (void) const override;
    QUrl    summaryQmlSource        (void) const override;
    bool    allowSetupWhileArmed    (void) const override { return true; }

private:
    const QString   _name;
    QVariantList    _summaryItems;
};

#endif
