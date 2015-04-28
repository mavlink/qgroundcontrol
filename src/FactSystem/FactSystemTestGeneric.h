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

#ifndef FactSystemTestGeneric_H
#define FactSystemTestGeneric_H

#include "FactSystemTestBase.h"
#include "UASInterface.h"
#include "AutoPilotPlugin.h"

// Unit Test for Fact System on PX4 autopilot
class FactSystemTestGeneric : public FactSystemTestBase
{
    Q_OBJECT
    
public:
    FactSystemTestGeneric(void);
    
private slots:
    void init(void);
    void cleanup(void) { _cleanup(); }
    
    void parameter_default_component_id_test(void) { _parameter_default_component_id_test(); }
    void parameter_specific_component_id_test(void) { _parameter_specific_component_id_test(); }
    void qml_test(void) { _qml_test(); }
    void qmlUpdate_test(void) { _qmlUpdate_test(); }
};

#endif
