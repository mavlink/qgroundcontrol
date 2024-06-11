/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "VehicleComponent.h"

/// @file
///     @brief The Sensors VehicleComponent is used to calibrate the the various sensors associated with the board.
///     @author Don Gagne <don@thegagnes.com>

class SensorsComponent : public VehicleComponent
{
    Q_OBJECT
    
public:
    SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);
    
    Q_PROPERTY(bool airspeedCalSupported    READ _airspeedCalSupported  STORED false NOTIFY setupCompleteChanged)
    Q_PROPERTY(bool airspeedCalRequired     READ _airspeedCalRequired   STORED false NOTIFY setupCompleteChanged)

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
    bool _airspeedCalSupported  (void) const;
    bool _airspeedCalRequired   (void) const;

    const QString   _name;
    QVariantList    _summaryItems;
    QStringList     _deviceIds;
    QStringList     _airspeedCalTriggerParams;

    static const char* _magEnabledParam;
    static const char* _magCalParam;
};
