#pragma once

#include "UnitTest.h"

class GuardedTransitionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGuardedTransitionAllowed();
    void _testGuardedTransitionBlocked();
};
