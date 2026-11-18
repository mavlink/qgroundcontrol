#pragma once

#include "VehicleTestManualConnect.h"
#include "Vehicle.h"

/// Tests for RetryableRequestMessageState
/// These tests require MockLink infrastructure for full Vehicle integration.
class RetryableRequestMessageStateTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testSuccessFirstAttempt();
    void _testRetryOnFailure();
    void _testRetryAfterTimeout_data();
    void _testRetryAfterTimeout();
    void _testMaxRetriesExhausted();
    void _testFailOnMaxRetries();

private:
    /// Waits until the vehicle's own requests to the autopilot are done, so a test request is sent right away.
    bool _waitForNoAutopilotRequests();
    /// Sends the reply the vehicle has withheld: the command ack and the DEBUG message.
    void _sendWithheldDebugReply();
};
