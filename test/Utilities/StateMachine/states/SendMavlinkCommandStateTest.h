#pragma once

#include "UnitTest.h"

/// Tests for SendMavlinkCommandState
/// Note: Full integration tests require a MockLink/Vehicle connection
class SendMavlinkCommandStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testDeferredSetup();
    void _testUnconfiguredStateFails();
};
