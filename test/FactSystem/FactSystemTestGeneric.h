#pragma once

#include "FactSystemTestBase.h"

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
