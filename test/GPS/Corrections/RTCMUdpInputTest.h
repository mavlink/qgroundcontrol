#pragma once

#include "UnitTest.h"

class RTCMUdpInputTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testSocketErrors_data();
    void _testSocketErrors();
    void _testSenderInstanceNaming_data();
    void _testSenderInstanceNaming();
    void _testEmitsOneSignalPerFrame();
    void _testFrameSplitAcrossDatagrams();
    void _testInterleavedSenders();
    void _testBurstYieldsBetweenDrains();
};
