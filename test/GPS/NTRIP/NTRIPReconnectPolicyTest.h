#pragma once

#include "UnitTest.h"

class NTRIPReconnectPolicyTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testExponentialBackoff();
    void testMaxBackoff();
    void testCancelStopsTimer();
    void testResetAttempts();
    void testReconnectSignal();
};
