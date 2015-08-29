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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FactSystemTestBase_H
#define FactSystemTestBase_H

#include "UnitTest.h"
#include "UASInterface.h"
#include "AutoPilotPlugin.h"

// Base class for FactSystemTest[PX4|Generic] unit tests
class FactSystemTestBase : public UnitTest
{
    Q_OBJECT
    
public:
    FactSystemTestBase(void);
    
protected:
    void _init(MAV_AUTOPILOT autopilot);
    void _cleanup(void);
    
    void _parameter_default_component_id_test(void);
    void _parameter_specific_component_id_test(void);
    void _qml_test(void);
    void _qmlUpdate_test(void);
    
    AutoPilotPlugin*                _plugin;
};

#endif
