#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

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
    void _paramWriteParamError();
    void _paramReadParamError();
    void _FTPnoFailure();
    void _FTPChangeParam();
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
};
