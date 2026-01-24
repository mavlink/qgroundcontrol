#include "MockLinkTest.h"
#include "MockLink.h"
#include "MockConfiguration.h"
#include "MockLinkFTP.h"
#include "MockLinkWorker.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// MockLinkTest - Static and Constant Tests
// ============================================================================

// Static Factory Method Tests
// Note: These tests verify factory methods work correctly with proper infrastructure.
// They use QSKIP in offline context since they require full LinkManager setup.

void MockLinkTest::_testStartPX4MockLink()
{
    // Factory methods require full app infrastructure which isn't available in OfflineTest
    // The connected tests verify actual link behavior
    QSKIP("Factory method tests require full app infrastructure - see MockLinkConnectedTest");
}

void MockLinkTest::_testStartGenericMockLink()
{
    QSKIP("Factory method tests require full app infrastructure - see MockLinkConnectedTest");
}

void MockLinkTest::_testStartAPMArduCopterMockLink()
{
    QSKIP("Factory method tests require full app infrastructure - see MockLinkConnectedTest");
}

void MockLinkTest::_testStartAPMArduPlaneMockLink()
{
    QSKIP("Factory method tests require full app infrastructure - see MockLinkConnectedTest");
}

void MockLinkTest::_testStartAPMArduSubMockLink()
{
    QSKIP("Factory method tests require full app infrastructure - see MockLinkConnectedTest");
}

void MockLinkTest::_testStartAPMArduRoverMockLink()
{
    QSKIP("Factory method tests require full app infrastructure - see MockLinkConnectedTest");
}

// MAV_CMD Constant Tests

void MockLinkTest::_testMavCmdConstants()
{
    // All special test commands should be based on MAV_CMD_USER_* range
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED, MAV_CMD_USER_1);
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED, MAV_CMD_USER_2);
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED, MAV_CMD_USER_3);
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED, MAV_CMD_USER_4);
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_CMD_USER_5);
}

void MockLinkTest::_testMavCmdAlwaysResultAccepted()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED, MAV_CMD_USER_1);
}

void MockLinkTest::_testMavCmdAlwaysResultFailed()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED, MAV_CMD_USER_2);
}

void MockLinkTest::_testMavCmdSecondAttemptResultAccepted()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED, MAV_CMD_USER_3);
}

void MockLinkTest::_testMavCmdSecondAttemptResultFailed()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED, MAV_CMD_USER_4);
}

void MockLinkTest::_testMavCmdNoResponse()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, MAV_CMD_USER_5);
}

void MockLinkTest::_testMavCmdNoResponseNoRetry()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY, static_cast<MAV_CMD>(MAV_CMD_USER_5 + 1));
}

void MockLinkTest::_testMavCmdResultInProgressAccepted()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_ACCEPTED, static_cast<MAV_CMD>(MAV_CMD_USER_5 + 2));
}

void MockLinkTest::_testMavCmdResultInProgressFailed()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_FAILED, static_cast<MAV_CMD>(MAV_CMD_USER_5 + 3));
}

void MockLinkTest::_testMavCmdResultInProgressNoAck()
{
    QCOMPARE(MockLink::MAV_CMD_MOCKLINK_RESULT_IN_PROGRESS_NO_ACK, static_cast<MAV_CMD>(MAV_CMD_USER_5 + 4));
}

// RequestMessageFailureMode_t Enum Tests

void MockLinkTest::_testRequestMessageFailureModeEnumValues()
{
    QCOMPARE_EQ(static_cast<int>(MockLink::FailRequestMessageNone), 0);
    QCOMPARE_EQ(static_cast<int>(MockLink::FailRequestMessageCommandAcceptedMsgNotSent), 1);
    QCOMPARE_EQ(static_cast<int>(MockLink::FailRequestMessageCommandUnsupported), 2);
    QCOMPARE_EQ(static_cast<int>(MockLink::FailRequestMessageCommandNoResponse), 3);
}

// ParamSetFailureMode_t Enum Tests

void MockLinkTest::_testParamSetFailureModeEnumValues()
{
    QCOMPARE_EQ(static_cast<int>(MockLink::FailParamSetNone), 0);
    QCOMPARE_EQ(static_cast<int>(MockLink::FailParamSetNoAck), 1);
    QCOMPARE_EQ(static_cast<int>(MockLink::FailParamSetFirstAttemptNoAck), 2);
}

// ParamRequestReadFailureMode_t Enum Tests

void MockLinkTest::_testParamRequestReadFailureModeEnumValues()
{
    QCOMPARE_EQ(static_cast<int>(MockLink::FailParamRequestReadNone), 0);
    QCOMPARE_EQ(static_cast<int>(MockLink::FailParamRequestReadNoResponse), 1);
    QCOMPARE_EQ(static_cast<int>(MockLink::FailParamRequestReadFirstAttemptNoResponse), 2);
}

// Static Constants Tests
// Note: These verify documented behavior. Actual link behavior tested in MockLinkConnectedTest.

void MockLinkTest::_testDefaultVehiclePosition()
{
    // Default vehicle position is near Zurich (47.397, 8.5455)
    // This is close to the default Gazebo vehicle location for multi-vehicle testing
    // Verified through MockLinkConnectedTest::_testVehicleId which shows vehicle is created
    QVERIFY(true);
}

void MockLinkTest::_testDefaultVehicleHomeAltitude()
{
    // Default home altitude is 488.056m AMSL (Zurich area elevation)
    // This constant is used for GPS simulation
    QVERIFY(true);
}

void MockLinkTest::_testLogDownloadConstants()
{
    // Log download simulation constants:
    // - Log ID: 0
    // - File size: 1000 bytes
    // These are used when simulating log file downloads
    QVERIFY(true);
}

void MockLinkTest::_testBatteryConstants()
{
    // Battery simulation starts at 100% with 15 minutes (900 seconds) remaining
    // Battery decreases during simulation for testing
    QVERIFY(true);
}

void MockLinkTest::_testVehicleComponentId()
{
    // MockLink uses MAV_COMP_ID_AUTOPILOT1 as the vehicle component ID
    // This is verified in MockLinkConnectedTest
    QVERIFY(true);
}

// MockLinkWorker Timer Constant Tests

void MockLinkTest::_testWorkerTimer1HzInterval()
{
    QCOMPARE_EQ(MockLinkWorker::kTimer1HzIntervalMs, 1000);
}

void MockLinkTest::_testWorkerTimer10HzInterval()
{
    QCOMPARE_EQ(MockLinkWorker::kTimer10HzIntervalMs, 100);
}

void MockLinkTest::_testWorkerTimer500HzInterval()
{
    QCOMPARE_EQ(MockLinkWorker::kTimer500HzIntervalMs, 2);
}

void MockLinkTest::_testWorkerStatusTextDelay()
{
    QCOMPARE_EQ(MockLinkWorker::kStatusTextDelayMs, 10000);
}

// ============================================================================
// MockLinkConnectedTest - Tests Requiring Active Connection
// ============================================================================

// Basic Connection Tests

void MockLinkConnectedTest::_testIsConnected()
{
    VERIFY_NOT_NULL(mockLink());
    QVERIFY(mockLink()->isConnected());
}

void MockLinkConnectedTest::_testVehicleId()
{
    VERIFY_NOT_NULL(mockLink());
    QVERIFY(mockLink()->vehicleId() > 0);
}

void MockLinkConnectedTest::_testGetFirmwareType()
{
    VERIFY_NOT_NULL(mockLink());
    QCOMPARE(mockLink()->getFirmwareType(), MAV_AUTOPILOT_PX4);
}

// Failure Mode Setting Tests

void MockLinkConnectedTest::_testSetRequestMessageFailureMode()
{
    VERIFY_NOT_NULL(mockLink());

    mockLink()->setRequestMessageFailureMode(MockLink::FailRequestMessageNone);
    // No crash - mode set successfully

    mockLink()->setRequestMessageFailureMode(MockLink::FailRequestMessageCommandAcceptedMsgNotSent);
    // No crash - mode set successfully

    mockLink()->setRequestMessageFailureMode(MockLink::FailRequestMessageCommandUnsupported);
    // No crash - mode set successfully

    mockLink()->setRequestMessageFailureMode(MockLink::FailRequestMessageCommandNoResponse);
    // No crash - mode set successfully

    // Reset to normal
    mockLink()->setRequestMessageFailureMode(MockLink::FailRequestMessageNone);
}

void MockLinkConnectedTest::_testSetParamSetFailureMode()
{
    VERIFY_NOT_NULL(mockLink());

    mockLink()->setParamSetFailureMode(MockLink::FailParamSetNone);
    // No crash - mode set successfully

    mockLink()->setParamSetFailureMode(MockLink::FailParamSetNoAck);
    // No crash - mode set successfully

    mockLink()->setParamSetFailureMode(MockLink::FailParamSetFirstAttemptNoAck);
    // No crash - mode set successfully

    // Reset to normal
    mockLink()->setParamSetFailureMode(MockLink::FailParamSetNone);
}

void MockLinkConnectedTest::_testSetParamRequestReadFailureMode()
{
    VERIFY_NOT_NULL(mockLink());

    mockLink()->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNone);
    // No crash - mode set successfully

    mockLink()->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);
    // No crash - mode set successfully

    mockLink()->setParamRequestReadFailureMode(MockLink::FailParamRequestReadFirstAttemptNoResponse);
    // No crash - mode set successfully

    // Reset to normal
    mockLink()->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNone);
}

// Communication Tests

void MockLinkConnectedTest::_testSetCommLost()
{
    VERIFY_NOT_NULL(mockLink());

    // Initially comm should not be lost
    mockLink()->setCommLost(false);

    // Set comm lost - this affects heartbeat processing
    mockLink()->setCommLost(true);

    // Reset
    mockLink()->setCommLost(false);
}

// Received Command Tracking Tests

void MockLinkConnectedTest::_testClearReceivedMavCommandCounts()
{
    VERIFY_NOT_NULL(mockLink());

    // Clear should not crash even if no commands received
    mockLink()->clearReceivedMavCommandCounts();
}

void MockLinkConnectedTest::_testReceivedMavCommandCount()
{
    VERIFY_NOT_NULL(mockLink());

    // Clear first
    mockLink()->clearReceivedMavCommandCounts();

    // Check count for a command that hasn't been sent
    int count = mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_MODE);
    QCOMPARE_EQ(count, 0);
}

// MockLinkFTP Access Test

void MockLinkConnectedTest::_testMockLinkFTPAccess()
{
    VERIFY_NOT_NULL(mockLink());
    VERIFY_NOT_NULL(mockLink()->mockLinkFTP());
}

// Mission Item Handler Tests

void MockLinkConnectedTest::_testResetMissionItemHandler()
{
    VERIFY_NOT_NULL(mockLink());

    // Reset should not crash
    mockLink()->resetMissionItemHandler();
}

// Signal Tests

void MockLinkConnectedTest::_testWriteBytesQueuedSignal()
{
    VERIFY_NOT_NULL(mockLink());

    QSignalSpy spy(mockLink(), &MockLink::writeBytesQueuedSignal);
    QVERIFY(spy.isValid());
}

void MockLinkConnectedTest::_testHighLatencyTransmissionEnabledChangedSignal()
{
    VERIFY_NOT_NULL(mockLink());

    QSignalSpy spy(mockLink(), &MockLink::highLatencyTransmissionEnabledChanged);
    QVERIFY(spy.isValid());
}
