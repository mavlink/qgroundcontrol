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
    void _paramWriteNoAckRetry();
    void _paramWriteNoAckPermanent();
    void _paramReadFirstAttemptNoResponseRetry();
    void _paramReadNoResponse();
    // void _FTPnoFailure();
    // void _FTPChangeParam();

private:
    void _noFailureWorker(MockConfiguration::FailureMode_t failureMode);
    void _setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess);
};
