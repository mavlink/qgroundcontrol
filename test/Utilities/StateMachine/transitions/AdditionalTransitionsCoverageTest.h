#pragma once

#include "StateMachineTest.h"

class AdditionalTransitionsCoverageTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testQGCEventTransitionMatchesEvent();
    void _testQGCEventTransitionGuardBlocksThenFallbackTakesEvent();
    void _testQGCAbstractTransitionAccessorsAndCustomEventTransition();
};
