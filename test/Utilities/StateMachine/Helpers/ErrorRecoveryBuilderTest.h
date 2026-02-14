#pragma once

#include "UnitTest.h"

class ErrorRecoveryBuilderTest : public UnitTest
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
