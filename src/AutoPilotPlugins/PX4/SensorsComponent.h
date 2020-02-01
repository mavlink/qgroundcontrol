/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList(void) const override;
    
    // Virtuals from VehicleComponent
    virtual QString name(void) const override;
    virtual QString description(void) const override;
    virtual QString iconResource(void) const override;
    virtual bool requiresSetup(void) const override;
    virtual bool setupComplete(void) const override;
    virtual QUrl setupSource(void) const override;
    virtual QUrl summaryQmlSource(void) const override;
    
private:
    const QString   _name;
    QVariantList    _summaryItems;
    QStringList     _deviceIds;

    static const char* _airspeedDisabledParam;
    static const char* _airspeedBreakerParam;
    static const char* _airspeedCalParam;

    static const char* _magEnabledParam;
    static const char* _magCalParam;
};

#endif
