#pragma once

#include "UnitTest.h"

/// When StreamWatchdog takes a stream for stalled, on times passed in.
class StreamWatchdogTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStallDetection_data();
    void _testStallDetection();
    void _testFirstFrameGrace_data();
    void _testFirstFrameGrace();
    void _testSinkSilenceCountsFromItsClock();
    void _testSinkLimits();
};
