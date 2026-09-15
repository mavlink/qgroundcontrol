#pragma once

#include "UnitTest.h"

class RTCMUdpInputTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStartStop();
    void _testSocketErrors_data();
    void _testSocketErrors();
    void _testStartNotificationReentrancy_data();
    void _testStartNotificationReentrancy();
    void _testStartupPortReplacement_data();
    void _testStartupPortReplacement();
    void _testValidationResetsStream();
    void _testReentrantDrainPreservesOrder();
    void _testDrainInterruption_data();
    void _testDrainInterruption();
    void _testPassthroughWithoutValidation();
    void _testEmitsOneSignalPerFrame();
    void _testDropsBadCrcFrame();
    void _testRecoversBufferedFrames();
    void _testFrameSplitAcrossDatagrams();
    void _testInterleavedSenders();
    void _testBurstYieldsBetweenDrains();
};
