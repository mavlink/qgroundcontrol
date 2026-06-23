#pragma once

#include "StateMachineTest.h"

/// Tests for WaitForMavlinkMessageState
/// Note: Full integration tests require a MockLink/Vehicle connection
class WaitForMavlinkMessageStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testMessageId();
    void _testPredicateSetup();
    void _testTimeoutConfiguration();
};
