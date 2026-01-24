#pragma once

#include "TestFixtures.h"

/// Unit tests for MultiSignalSpy class.
/// Tests signal monitoring, counting, clearing, and error handling.
class MultiSignalSpyTest : public OfflineTest
{
    Q_OBJECT

public:
    MultiSignalSpyTest() = default;

private slots:
    // Initialization tests
    void _testInitWithNullEmitter();
    void _testInitWithValidEmitter();
    void _testInitWithSpecificSignals();
    void _testInitWithEmptySignalList();
    void _testInitWithInvalidSignalName();
    void _testInitMaxSignalLimit();
    void _testReInitCleansUp();

    // Constants tests
    void _testMaxSignalsConstant();

    // isValid tests
    void _testIsValidAfterInit();
    void _testIsValidBeforeInit();

    // Signal checking tests
    void _testCheckSignalNotEmitted();
    void _testCheckSignalEmittedOnce();
    void _testCheckSignalEmittedMultiple();
    void _testCheckOnlySignal();
    void _testCheckNoSignal();
    void _testCheckNoSignals();

    // Mask tests
    void _testMaskForKnownSignal();
    void _testMaskForUnknownSignal();
    void _testMaskCombination();

    // Clear tests
    void _testClearSignal();
    void _testClearAllSignals();
    void _testClearSignalsByMask();

    // Wait tests
    void _testWaitForSignalAlreadyEmitted();

    // Access tests
    void _testSpyForKnownSignal();
    void _testSpyForUnknownSignal();
    void _testCountForSignal();
    void _testSignalNames();
    void _testSignalCount();
    void _testHasSignal();

    // Summary tests
    void _testSummaryNoSignals();
    void _testSummaryWithSignals();
    void _testTotalEmissions();
    void _testUniqueSignalsEmitted();

    // Expectation API tests
    void _testExpectOnce();
    void _testExpectAtLeastOnce();
    void _testExpectNever();
    void _testExpectTimes();

    // Argument extraction tests
    void _testArgumentExtraction();
    void _testPullBoolFromSignal();
    void _testPullIntFromSignal();
};
