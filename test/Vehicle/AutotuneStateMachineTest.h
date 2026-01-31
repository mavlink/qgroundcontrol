#pragma once

#include "UnitTest.h"

class AutotuneStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testStartAutotune();
    void _testProgressTransitions();
    void _testFailure();
    void _testError();
    void _testFullWorkflow();
};
