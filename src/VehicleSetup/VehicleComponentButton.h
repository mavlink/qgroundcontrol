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

#ifndef VEHICLECOMPONENTBUTTON_H
#define VEHICLECOMPONENTBUTTON_H

#include "VehicleSetupButton.h"
#include "VehicleComponent.h"

/// @file
///     @brief This class is used for the push buttons in the Vehicle Setup display.
///     @author Don Gagne <don@thegagnes.com>

class VehicleComponentButton : public VehicleSetupButton
{
    Q_OBJECT
    
public:
    /// @param component VehicleComponent associated witht this button
    VehicleComponentButton(VehicleComponent* component, QWidget* parent = NULL) :
        VehicleSetupButton(parent),
        _component(component)
    {
        Q_ASSERT(component);
    }
    
    /// @brief Returns the component associated with the button
    VehicleComponent* component(void) { return _component; }
    
private:
    VehicleComponent* _component;
};

#endif
