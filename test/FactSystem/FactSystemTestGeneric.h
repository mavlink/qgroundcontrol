#pragma once

#include "FactSystemTestBase.h"

// Unit Test for Fact System on PX4 autopilot
class FactSystemTestGeneric : public FactSystemTestBase
{
    Q_OBJECT

private slots:
    void init();

    void cleanup()
    {
        _cleanup();
    }

    void parameter_default_component_id_test()
    {
        _parameter_default_component_id_test();
    }

    void parameter_specific_component_id_test()
    {
        _parameter_specific_component_id_test();
    }

    void qml_test()
    {
        _qml_test();
    }

    void qmlUpdate_test()
    {
        _qmlUpdate_test();
    }
};
