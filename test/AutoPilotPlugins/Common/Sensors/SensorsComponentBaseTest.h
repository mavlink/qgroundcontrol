#pragma once

#include "UnitTest.h"

class SensorsComponentBaseTest : public UnitTest
{
    Q_OBJECT

public:
    SensorsComponentBaseTest() = default;

private slots:
    void _testName();
    void _testDescription();
    void _testIconResource();
    void _testRequiresSetup();
    void _testParameterHelpers();
};
