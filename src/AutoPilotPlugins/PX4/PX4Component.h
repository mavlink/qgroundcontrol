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

#ifndef PX4COMPONENT_H
#define PX4COMPONENT_H

#include "VehicleComponent.h"

/// @file
///     @brief This class is used as an abstract base class for all PX4 VehicleComponent objects.
///     @author Don Gagne <don@thegagnes.com>

class PX4Component : public VehicleComponent
{
    Q_OBJECT
    
public:
    PX4Component(UASInterface* uas, AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    /// @brief Returns an array of parameter names for which a change should cause the setupCompleteChanged
    ///         signal to be emitted. Last element is signalled by NULL. Must be implemented by upper level class.
    virtual const char** setupCompleteChangedTriggerList(void) const = 0;
    
private slots:
    /// @brief Connected to QGCUASParamManagerInterface::parameterUpdated signal in order to signal
    ///         setupCompleteChanged at appropriate times.
    void _parameterUpdated(int compId, QString paramName, QVariant value);
};

#endif
