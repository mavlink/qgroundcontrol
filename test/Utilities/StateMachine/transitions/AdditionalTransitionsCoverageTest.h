#pragma once

#include "UnitTest.h"

class AdditionalTransitionsCoverageTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testQGCEventTransitionMatchesEvent();
    void _testQGCEventTransitionGuardBlocksThenFallbackTakesEvent();
    void _testQGCAbstractTransitionAccessorsAndCustomEventTransition();
};
