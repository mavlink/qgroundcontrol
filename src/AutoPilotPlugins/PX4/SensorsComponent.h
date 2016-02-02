/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
};

#endif
