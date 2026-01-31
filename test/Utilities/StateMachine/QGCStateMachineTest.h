#pragma once

#include "UnitTest.h"

class QGCStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    // QGCStateMachine helpers
    void _testQGCStateMachineFactories();
    void _testGlobalErrorState();
    void _testLocalErrorState();

    // Property assignment
    void _testPropertyAssignment();

    // Transition builders
    void _testTimeoutTransitionBuilder();
    void _testRetryTransitionBuilder();
    void _testConditionalTransitionBuilder();

    // State introspection
    void _testTransitionsFrom();
    void _testTransitionsTo();
    void _testReachableFrom();
    void _testPredecessorsOf();

    // Composite state patterns
    void _testTimedActionState();
    void _testTimedActionStateWithCallbacks();
    void _testSelfLoopTransition();
    void _testInternalTransition();
};
