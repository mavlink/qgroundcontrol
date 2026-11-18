#pragma once

#include "VehicleTestManualConnect.h"

/// Tests for SendMavlinkCommandState on a MockLink vehicle
class SendMavlinkCommandStateTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testCommandResult);
    void _testIgnoresResultOfOtherCommand();
    void _testUnconfiguredStateFails();
};
