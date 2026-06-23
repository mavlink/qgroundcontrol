#pragma once

#include "StateMachineTest.h"

/// Tests for SendMavlinkMessageState
/// Note: Full integration tests require a MockLink/Vehicle connection
class SendMavlinkMessageStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testEncoderCallback();
};
