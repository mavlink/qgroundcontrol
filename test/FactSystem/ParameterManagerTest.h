#pragma once

#include "UnitTest.h"
#include "MockConfiguration.h"
#include "MockLink.h"

class ParameterManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _noFailure(void);
    void _requestListNoResponse(void);
    void _requestListMissingParamSuccess(void);
    void _requestListMissingParamFail(void);
    void _paramWriteNoAckRetry(void);
    void _paramWriteNoAckPermanent(void);
    void _paramReadFirstAttemptNoResponseRetry(void);
    void _paramReadNoResponse(void);
    void _skipParameterDownload(void);
    // void _FTPnoFailure(void);
    // void _FTPChangeParam(void);


private:
    void _noFailureWorker(MockConfiguration::FailureMode_t failureMode);
    void _setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess);
};
