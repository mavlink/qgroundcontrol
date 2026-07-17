#pragma once

#include "BaseClasses/VehicleTest.h"
#include "Vehicle.h"

/// Reproduces issue #13251 (crash 3): out-of-bounds list access in
/// MavCommandQueue::_responseTimeoutCheck. When a command gives up after max retries,
/// the result handler is invoked from within the timeout-check loop. If that handler
/// re-enters the queue and clears the pending list (which is exactly what happens when
/// the user closes log replay or disconnects the vehicle, via
/// VehicleLinkManager::closeVehicle -> Vehicle::_stopCommandProcessing -> MavCommandQueue::stop),
/// the loop continues iterating stale indices into the now-empty list.
class MavCommandQueueReentrancyTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

public:
    explicit MavCommandQueueReentrancyTest(QObject* parent = nullptr)
        : VehicleTestNoInitialConnect(parent)
    {
        setAutopilotType(MAV_AUTOPILOT_INVALID);
    }

private slots:
    void _testCloseVehicleFromGiveUpHandler();

private:
    static void _closeVehicleResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack,
                                           Vehicle::MavCmdResultFailureCode_t failureCode);

    int _resultHandlerCallCount = 0;
};
