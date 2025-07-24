/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class AutoPilotPlugin;

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

