#pragma once

#include "UnitTest.h"

class EdgeTriggeredCounterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFiresOnceWhenThresholdIsReached();
    void _testResetStartsNewBurst();
};
