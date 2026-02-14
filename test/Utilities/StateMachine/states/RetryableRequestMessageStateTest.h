#pragma once

#include "VehicleTestManualConnect.h"
#include "MAVLinkLib.h"
#include "Vehicle.h"

/// Tests for RetryableRequestMessageState
/// These tests require MockLink infrastructure for full Vehicle integration.
class RetryableRequestMessageStateTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testSuccessFirstAttempt();
    void _testRetryOnFailure();
    void _testMaxRetriesExhausted();
    void _testFailOnMaxRetries();
};
