#pragma once

#include "UnitTest.h"

class RTCMUdpInputTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStartStop();
    void _testStartNotificationReentrancy_data();
    void _testStartNotificationReentrancy();
    void _testPassthroughWithoutValidation();
    void _testEmitsOneSignalPerFrame();
    void _testDropsBadCrcFrame();
    void _testFrameSplitAcrossDatagrams();
    void _testInterleavedSenders();
    void _testBurstYieldsBetweenDrains();
};
