#pragma once

#include "VehicleTestManualConnect.h"

/// Tests for SendMavlinkMessageState on a MockLink vehicle
class SendMavlinkMessageStateTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testSendsEncodedMessage();
    void _testFailsOnceRetriesAreUsedUp();
    void _testSendsAgainInNextRun();
};
