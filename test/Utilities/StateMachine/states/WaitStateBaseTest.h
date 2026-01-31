#pragma once

#include "UnitTest.h"

class WaitStateBaseTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testTimeoutEmission();
    void _testWaitComplete();
    void _testWaitFailed();
    void _testNoTimeoutWhenZero();
    void _testDoubleCompleteProtection();
    void _testDoubleFailProtection();
    void _testTimeoutCancelledOnComplete();
    void _testSignalDisconnectOnExit();
};
