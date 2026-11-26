#pragma once

#include "BaseClasses/VehicleTest.h"

/// Tests that MessageIntervalManager::setMessageRate() doesn't re-request messages a component refused.
class MessageIntervalManagerTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

public:
    explicit MessageIntervalManagerTest(QObject* parent = nullptr)
        : VehicleTestNoInitialConnect(parent)
    {
        setAutopilotType(MAV_AUTOPILOT_INVALID);
    }

private slots:
    void _refusedRateNotRequestedAgain();
    void _refusedRateOtherRateRequested();
    void _acceptedRateRequestedAgain();
    void _acceptedRatesConfirmedOneAtATime();
    void _overlappingRefusalsRemembered();
    void _refusalAfterLostAckNotRemembered();

private:
    /// Calls setMessageRate() and waits for its SET_MESSAGE_INTERVAL to be acked
    void _setMessageRate(Vehicle* vehicle, uint16_t msgId, int32_t rate = 1);

    /// Waits for no SET_MESSAGE_INTERVAL to be pending, then clears MockLink's command counts
    void _waitIdle(Vehicle* vehicle);
};
