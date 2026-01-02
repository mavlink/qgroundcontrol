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
