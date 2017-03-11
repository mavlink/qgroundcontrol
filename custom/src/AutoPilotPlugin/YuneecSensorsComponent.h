/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SensorsComponent.h"

/// @file
///     @brief The Sensors VehicleComponent is used to calibrate the the various sensors associated with the board.
///     @author Don Gagne <don@thegagnes.com>

class YuneecSensorsComponent : public SensorsComponent
{
    Q_OBJECT
    
public:
    YuneecSensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = NULL) :
        SensorsComponent(vehicle, autopilot, parent) { }

    // Virtuals from VehicleComponent

    // No pre-reqs for Yuneec
    virtual QString prerequisiteSetup(void) const final { return QString(); }
};
