#pragma once

#include "UnitTest.h"

class ComponentInformationManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testProgressTracking();
    void _testSkipUnsupportedTypes();
    void _testArduPilotComponentInfo();
    void _testCompletionSignal();
    void _testMultipleVehicles();
};
