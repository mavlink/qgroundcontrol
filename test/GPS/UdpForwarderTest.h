#pragma once

#include "UnitTest.h"

class UdpForwarderTest : public UnitTest
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
