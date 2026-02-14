#pragma once

#include "UnitTest.h"

class ProgressStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFixedProgress();
    void _testProgressCallback();
    void _testProgressClamping();
    void _testProgressSequence();
    void _testActionExecuted();
};
