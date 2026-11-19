#pragma once

#include <memory>

#include "BaseClasses/VehicleTestManualConnect.h"
#include "ManualScheduler.h"
#include "MockLink.h"

class ParameterManagerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void cleanup() override;

    void _noFailure();
    void _requestListNoResponse();
    void _requestListMissingParamSuccess();
    void _requestListMissingParamFail();
    void _requestListMissingParamNonDefaultComponentFail();
    void _requestListNonDefaultComponentDead();
    void _requestListNonDefaultComponentLossy();
    void _requestListNonDefaultComponentDoesNotGateReady();
    void _refreshAllViaFtpKeepsProgressDownForBackgroundComponent();
    void _refreshAllViaStreamClearsProgressAfterGiveUp();
    void _requestListSharedIndexAcrossComponents();
    void _paramWriteNoAckRetry();
    void _paramWriteNoAckPermanent();
    void _paramWriteUInt8();
    void _paramWriteUInt16();
    void _paramReadFirstAttemptNoResponseRetry();
    void _paramReadNoResponse();
    void _requestStateMachineDeletedWhenFinished_data();
    void _requestStateMachineDeletedWhenFinished();
    void _paramWriteParamError();
    void _paramReadParamError_data();
    void _paramReadParamError();
    void _rebootRequiredNotice_data();
    void _rebootRequiredNotice();
    void _FTPnoFailure();
    void _FTPChangeParam();
    void _FTPRequestsHeartbeatComponentParams_data();
    void _FTPRequestsHeartbeatComponentParams();
    void _bulkRefreshExactNamesAllSucceed();
    void _bulkRefreshPrefixExpansion();
    void _bulkRefreshUnknownNameSkipped();
    void _bulkRefreshRetrySucceeds();
    void _bulkRefreshAllRetriesExhausted();

private:
    void _ignoreParamResponseTimeouts();
    void _expectIndexLoadFailureWarning();
    void _noFailureWorker(MockConfiguration::FailureMode_t failureMode);
    Vehicle *_connectAndWaitForVehicle(MockConfiguration::FailureMode_t failureMode);
    static int _readCountForComponent(const QList<QPair<int, int>> &readLog, int componentId);
    /// Waits until an app message matching pattern has been logged. Non-default component failures are reported
    /// after parametersReady, so tests can't rely on the ready signal to sequence them.
    static bool _waitForAppMessage(const QRegularExpression &pattern);
    void _setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess,
                                  const QString &paramName, MAV_AUTOPILOT autopilot);
    /// Times what the parameter manager of @a vehicle starts from now on with a scheduler that only the test
    /// advances, so that an attempt ends at its timeout when the test says so
    void _timeRequestsByHand(Vehicle* vehicle);
    /// The same for the vehicle that connects next, from its creation: the vehicle can begin to load its parameters
    /// before it is the active one
    void _timeNextVehicleByHand();
    /// Lets the attempt of each outstanding request end at its ack timeout, once the vehicle has received
    /// @a receivedCount messages with @a requestMessageId and so has left the last of them unanswered
    void _expireAckTimeout(uint32_t requestMessageId, int receivedCount);

    /// Destroyed in cleanup(), once the vehicle that was given it is gone
    std::unique_ptr<ManualScheduler> _scheduler;
    QMetaObject::Connection _vehicleAddedConnection;
};
