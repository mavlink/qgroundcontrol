#pragma once

#include "UnitTest.h"

class QGCStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    // QGCStateMachine helpers
    void _testQGCStateMachineFactories();
    void _testGlobalErrorState();
    void _testGlobalErrorStateWithAbstractState();
    void _testLocalErrorState();

    // Property assignment
    void _testPropertyAssignment();

    // Transition builders
    void _testTimeoutTransitionBuilder();
    void _testRetryTransitionBuilder();
    void _testConditionalTransitionBuilder();
    void _testErrorRecoveryFactory();
    void _testErrorHandlerFactories();

    // State introspection
    void _testTransitionsFrom();
    void _testTransitionsTo();
    void _testReachableFrom();
    void _testPredecessorsOf();

    // Progress and state history
    void _testProgressTrackingWithSubProgress();
    void _testStateHistoryLimit();
    void _testRuntimeDiagnosticsHelpers();

    // Composite state patterns
    void _testTimedActionState();
    void _testTimedActionStateWithCallbacks();
    void _testSelfLoopTransition();
    void _testInternalTransition();
};
