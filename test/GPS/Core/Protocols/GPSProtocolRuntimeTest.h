#pragma once

#include "UnitTest.h"

/// The protocol runtime: allocation-free decoding, a new protocol for each configuration, readiness after a session
/// failure, deadlines and link rates.
class GPSProtocolRuntimeTest : public UnitTest
{
    Q_OBJECT

private slots:
    /// Steady-state u-blox decoding allocates nothing per epoch, beyond the bytes each RTCM3 frame owns.
    void _steadyStateDecodeDoesNotAllocate();
    void _readinessSeesSessionFailure();
    /// Each configure() runs on a new protocol object and a stream as the family describes it.
    void _configureStartsFromNewProtocol();
    /// A family without a protocol fails its configuration with a reason, like every other failure.
    void _configureWithoutProtocolSaysWhy();
    void _publishAsReceivedKeepsReceipt();
    void _deadlineRemaining_data();
    void _deadlineRemaining();
    void _deadlineAfter();
    /// The rate a family without a baud search runs the link at.
    void _linkBaud_data();
    void _linkBaud();
};
