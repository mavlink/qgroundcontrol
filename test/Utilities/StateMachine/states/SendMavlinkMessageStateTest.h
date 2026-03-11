#pragma once

#include "UnitTest.h"

/// Tests for SendMavlinkMessageState
/// Note: Full integration tests require a MockLink/Vehicle connection
class SendMavlinkMessageStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testEncoderCallback();
};
