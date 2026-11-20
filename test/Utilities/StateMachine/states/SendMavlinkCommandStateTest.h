#pragma once

#include "VehicleTestManualConnect.h"

/// Tests for SendMavlinkCommandState on a MockLink vehicle
class SendMavlinkCommandStateTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testCommandResult);
    UT_PARAMETERIZED_TEST(_testIgnoresResultThatIsNotItsOwn);
    UT_PARAMETERIZED_TEST(_testFailsWhenItsCommandGetsNoAck);
    void _testUnconfiguredStateFails();
};
