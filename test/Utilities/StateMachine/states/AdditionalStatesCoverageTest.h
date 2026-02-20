#pragma once

#include "UnitTest.h"

class AdditionalStatesCoverageTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCircuitBreakerTripsAndFailFast();
    void _testEventQueuedStateMatchesExpectedEvent();
    void _testFallbackChainUsesFallbackStrategy();
    void _testLoopStateProcessesFilteredItems();
    void _testRetryThenFailEmitsExhausted();
    void _testRetryThenSkipAdvancesAfterSkip();
    void _testRollbackStateExecutesRollbackOnFailure();
    void _testSequenceStateStopsOnFailedStep();
};
