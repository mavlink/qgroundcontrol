#pragma once

#include "UnitTest.h"

class NTRIPUdpForwarderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testConfigureValid();
    void testConfigureInvalid();
    void testForward();
    void testStop();
    void testReconfigure();
};
