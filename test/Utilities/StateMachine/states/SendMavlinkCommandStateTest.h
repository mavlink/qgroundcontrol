#pragma once

#include "StateMachineTest.h"

/// Tests for SendMavlinkCommandState
/// Note: Full integration tests require a MockLink/Vehicle connection
class SendMavlinkCommandStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testDeferredSetup();
    void _testUnconfiguredStateFails();
};
