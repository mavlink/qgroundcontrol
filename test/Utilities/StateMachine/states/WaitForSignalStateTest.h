#pragma once

#include "UnitTest.h"

class WaitForSignalStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testWaitForSignalState();
    void _testWaitForSignalStateTimeout();
    void _testCompletedSignal();
    void _testTimedOutSignal();
};
