#pragma once

#include "UnitTest.h"

class TimeoutTransitionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testTimeoutFires();
    void _testTimeoutCancelledOnExit();
    void _testTimeoutRestartsOnReentry();
    void _testMultipleTimeoutTransitions();
};
