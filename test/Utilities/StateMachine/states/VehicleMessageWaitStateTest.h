#pragma once

#include "VehicleTestManualConnect.h"

/// Tests for the request of a VehicleMessageWaitState
class VehicleMessageWaitStateTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testRequestReachesVehicle();
    void _testRequestIsSentAgainAfterEachTimeout();
    void _testAnswerToResentRequestCompletes();
    void _testRequestThatCannotBeSentFails();
};
