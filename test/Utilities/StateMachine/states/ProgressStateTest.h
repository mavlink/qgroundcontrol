#pragma once

#include "StateMachineTest.h"

class ProgressStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testFixedProgress();
    void _testProgressCallback();
    void _testProgressClamping();
    void _testProgressSequence();
    void _testActionExecuted();
};
