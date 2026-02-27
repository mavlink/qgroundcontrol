#pragma once

#include "BaseClasses/VehicleTest.h"

class AutoPilotPlugin;

// Base class for FactSystemTest[PX4|Generic] unit tests
class FactSystemTestBase : public VehicleTest
{
    Q_OBJECT

protected:
    void _init(MAV_AUTOPILOT autopilot);
    void _cleanup();

    void _parameter_default_component_id_test();
    void _parameter_specific_component_id_test();
    void _qml_test();
    void _qmlUpdate_test();

    AutoPilotPlugin* _plugin;
};
