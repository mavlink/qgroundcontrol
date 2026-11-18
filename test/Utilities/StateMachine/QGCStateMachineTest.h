#pragma once

#include "StateMachineTest.h"

class QGCStateMachineTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    // QGCStateMachine helpers
    void _testQGCStateMachineFactories();
    void _testGlobalErrorState();
    void _testGlobalErrorStateWithAbstractState();
    void _testLocalErrorState();
    UT_PARAMETERIZED_TEST(_testInProgressSpansQueuedStart);
    void _testStopFromInProgressListenerIsHonored();
    void _testStartWhileInProgressIsCritical();
    void _testStartBypassIsCritical();
    void _testSetInitialStateStartsMachine();
    UT_PARAMETERIZED_TEST(_testMachineAsStateOfAnotherMachine);

    // Property assignment
    void _testPropertyAssignment();

    // Transition builders
    void _testRetryTransitionBuilder();
    UT_PARAMETERIZED_TEST(_testGuardedTransitionBuilder);
    void _testTransitionBuildersTakeSignalOfDerivedState();

    // Progress and state history
    void _testProgressTrackingWithSubProgress();
    UT_PARAMETERIZED_TEST(_testRepeatedRestartRequests);
    UT_PARAMETERIZED_TEST(_testRestartResetsProgress);
    void _testRestartWhenFinishedResetsProgress();
    void _testStateHistoryLimit();
};
