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

#ifndef AUTOPILOTPLUGIN_H
#define AUTOPILOTPLUGIN_H

#include <QObject>
#include <QList>
#include <QString>

#include "UASInterface.h"
#include "VehicleComponent.h"

/// @file
///     @brief The AutoPilotPlugin class is an abstract base class which represent the methods and objects
///             which are specific to a certain AutoPilot. This is the only place where AutoPilot specific
///             code should reside in QGroundControl. The remainder of the QGroundControl source is
///             generic to a common mavlink implementation.
///     @author Don Gagne <don@thegagnes.com>

class AutoPilotPlugin : public QObject
{
    Q_OBJECT

public:
    /// @brief Returns the list of VehicleComponent objects associated with the AutoPilot.
    virtual QList<VehicleComponent*> getVehicleComponents(UASInterface* uas) const = 0;
    
    typedef struct {
        uint8_t baseMode;
        uint32_t customMode;
    } FullMode_t;
    
    /// @brief Returns the list of modes which are available for this AutoPilot.
    virtual QList<FullMode_t> getModes(void) const = 0;
    
    /// @brief Returns a human readable short description for the specified mode.
    virtual QString getShortModeText(uint8_t baseMode, uint32_t customMode) const = 0;
    
protected:
    // All access to AutoPilotPugin objects is through getInstanceForAutoPilotPlugin
    AutoPilotPlugin(QObject* parent = NULL) : QObject(parent) { }
};

#endif
