#pragma once

#include "StateMachineTest.h"

class ErrorRecoveryBuilderTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testPrimarySuccessAdvances();
    void _testFallbackAfterRetriesSucceeds();
    void _testFallbackFailureEndsInExhaustedError();
    void _testExhaustedEmitAdvanceRunsRollback();
    void _testExhaustedBehaviorMatrix();
    void _testTimeoutExhaustsWithoutExtraRetries();
};
