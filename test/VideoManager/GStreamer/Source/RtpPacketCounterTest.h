#pragma once

#include "UnitTest.h"

/// RtpPacketCounter on sequence numbers, timestamps and arrival times given by hand.
class RtpPacketCounterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testRtpPacketCounter_data();
    void _testRtpPacketCounter();
    void _testRtpPacketCounterDropoutFollowsPacketRate_data();
    void _testRtpPacketCounterDropoutFollowsPacketRate();
    void _testRtpPacketCounterJitter_data();
    void _testRtpPacketCounterJitter();
};
