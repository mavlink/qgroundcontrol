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
    void _testInProgressSpansQueuedStart_data();
    void _testInProgressSpansQueuedStart();
    void _testStopFromInProgressListenerIsHonored();
    void _testStartWhileInProgressIsCritical();
    void _testStartBypassIsCritical();

    // Property assignment
    void _testPropertyAssignment();

    // Transition builders
    void _testRetryTransitionBuilder();

    // Progress and state history
    void _testProgressTrackingWithSubProgress();
    void _testStateHistoryLimit();
};
