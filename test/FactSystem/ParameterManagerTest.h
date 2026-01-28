#pragma once

#include "UnitTest.h"
#include "TestHelpers.h"
#include "MockConfiguration.h"
#include "MockLink.h"

class Vehicle;
class MultiVehicleManager;

/// Tests for ParameterManager functionality.
/// Note: This test uses UnitTest directly (not ParameterTest) because it needs
/// fine-grained control over MockLink failure modes for each test case.
class ParameterManagerTest : public UnitTest
{
    Q_OBJECT

public:
    ParameterManagerTest() = default;

private slots:
    void _noFailure();
    void _requestListNoResponse();
    void _requestListMissingParamSuccess();
    void _requestListMissingParamFail();
    void _paramWriteNoAckRetry();
    void _paramWriteNoAckPermanent();
    void _paramReadFirstAttemptNoResponseRetry();
    void _paramReadNoResponse();
    void _FTPnoFailure();
    void _FTPChangeParam();

private:
    /// Helper to start MockLink and wait for vehicle to be available
    /// @param failureMode MockLink failure configuration
    /// @return Active vehicle, or nullptr on failure
    Vehicle* _startMockLinkAndWaitForVehicle(MockConfiguration::FailureMode_t failureMode);

    /// Helper for FTP-based parameter loading tests
    /// @return Active vehicle with FTP params loaded, or nullptr on failure
    Vehicle* _startFTPMockLinkAndWaitForParams();

    /// Worker for testing failure modes that should still succeed
    void _noFailureWorker(MockConfiguration::FailureMode_t failureMode);

    /// Worker for testing parameter set with various failure modes
    void _setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess);

    /// Calculate timeout based on ParameterManager retry constants
    static int _paramRetryTimeout(int retryCount, int baseTimeoutMs);
};
