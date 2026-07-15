#pragma once

#include "BaseClasses/VehicleTest.h"
#include "GCSControlManager.h"

class GCSControlManagerTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

public:
    explicit GCSControlManagerTest(QObject* parent = nullptr) : VehicleTestNoInitialConnect(parent)
    {
        setAutopilotType(MAV_AUTOPILOT_INVALID);
    }

private slots:
    void _controlStatusUpdatesState();
    void _controlStatusIgnoredWithoutSystemManagerFlag();
    void _controlStatusTracksTakeoverAllowedFlag();
    void _controlStatusGatesJoystickSend();
    void _controlStatusSecondaryListChangeEmitsSignal();
    void _requestOperatorControlRejectedByVehicle();
    void _requestOperatorControlPendingKeepsCountdown();
    void _requestOperatorControlAcceptedKeepsLockoutUntilConfirmed();
    void _requestOperatorControlLockoutClearedByTakeoverAllowedFlag();
    void _requestOperatorControlLockoutClearedWhenUncontrolled();
    void _requestOperatorControlLockoutExpiresAfterTimeout();
    void _requestOperatorControlDeniedShowsAuthorizationMessageAndClearsLockout();
    void _requestOperatorControlDuplicateRequestShowsWaitingMessage();
    void _startTimerRevertAllowTakeoverRequestsOnExpiry();
    void _releaseOperatorControlTargetsLearnedCompId();
    void _systemManagerCompIdChangeAdoptsNewManager();
    void _forwardedRequestOperatorControlEmitsSignalAndAcks();
    void _forwardedRequestSuppressedWhileTakeoverWindowActive();
    void _forwardedReleaseOperatorControlDoesNotEmitSignal();
};
