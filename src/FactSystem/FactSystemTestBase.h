/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
