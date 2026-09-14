#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class Fact;

class ParameterManagerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void cleanup() override;

    void _noFailure();
    void _requestListNoResponse();
    void _requestListMissingParamSuccess();
    void _requestListMissingParamFail();
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
    void _extParamsDownloaded();
    void _extParamWrite();
    void _extParamWriteRejected();
    void _extParamWriteNoAck();
    void _extParamWriteInProgress();
    void _extParamMissingIndexRetry();

private:
    /// Connects a camera-enabled MockLink and waits for the ext params to arrive.
    /// @return The CAM_EXPMODE fact, nullptr on failure
    Fact *_connectAndWaitForExtParams();
    void _ignoreParamResponseTimeouts();
    void _ignoreExtParamAckTimeouts();
    void _noFailureWorker(MockConfiguration::FailureMode_t failureMode);
    void _setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess,
                                  const QString &paramName, MAV_AUTOPILOT autopilot);
};
