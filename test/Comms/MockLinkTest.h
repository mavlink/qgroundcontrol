#pragma once

#include "TestFixtures.h"

/// Unit tests for MockLink.
/// Tests factory methods, failure modes, MAV_CMD constants, and link behavior.
class MockLinkTest : public OfflineTest
{
    Q_OBJECT

public:
    MockLinkTest() = default;

private slots:
    // Static factory method tests
    void _testStartPX4MockLink();
    void _testStartGenericMockLink();
    void _testStartAPMArduCopterMockLink();
    void _testStartAPMArduPlaneMockLink();
    void _testStartAPMArduSubMockLink();
    void _testStartAPMArduRoverMockLink();

    // MAV_CMD constant tests
    void _testMavCmdConstants();
    void _testMavCmdAlwaysResultAccepted();
    void _testMavCmdAlwaysResultFailed();
    void _testMavCmdSecondAttemptResultAccepted();
    void _testMavCmdSecondAttemptResultFailed();
    void _testMavCmdNoResponse();
    void _testMavCmdNoResponseNoRetry();
    void _testMavCmdResultInProgressAccepted();
    void _testMavCmdResultInProgressFailed();
    void _testMavCmdResultInProgressNoAck();

    // RequestMessageFailureMode_t enum tests
    void _testRequestMessageFailureModeEnumValues();

    // ParamSetFailureMode_t enum tests
    void _testParamSetFailureModeEnumValues();

    // ParamRequestReadFailureMode_t enum tests
    void _testParamRequestReadFailureModeEnumValues();

    // Static constants tests
    void _testDefaultVehiclePosition();
    void _testDefaultVehicleHomeAltitude();
    void _testLogDownloadConstants();
    void _testBatteryConstants();
    void _testVehicleComponentId();

    // MockLinkWorker timer constant tests
    void _testWorkerTimer1HzInterval();
    void _testWorkerTimer10HzInterval();
    void _testWorkerTimer500HzInterval();
    void _testWorkerStatusTextDelay();
};

/// Tests for MockLink that require an active connection.
/// Uses VehicleTest to have a connected MockLink available.
class MockLinkConnectedTest : public VehicleTest
{
    Q_OBJECT

public:
    MockLinkConnectedTest() = default;

private slots:
    // Basic connection tests
    void _testIsConnected();
    void _testVehicleId();
    void _testGetFirmwareType();

    // Failure mode setting tests
    void _testSetRequestMessageFailureMode();
    void _testSetParamSetFailureMode();
    void _testSetParamRequestReadFailureMode();

    // Communication tests
    void _testSetCommLost();

    // Received command tracking tests
    void _testClearReceivedMavCommandCounts();
    void _testReceivedMavCommandCount();

    // MockLinkFTP access test
    void _testMockLinkFTPAccess();

    // Mission item handler tests
    void _testResetMissionItemHandler();

    // Signals tests
    void _testWriteBytesQueuedSignal();
    void _testHighLatencyTransmissionEnabledChangedSignal();
};
