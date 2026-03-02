#pragma once

#include "UnitTest.h"

class NamedEventTransitionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testNamedEventTransition();
    void _testNamedEventTransitionWithGuard();
};
