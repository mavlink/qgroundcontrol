#include "InitialConnectPeripheralStartupTest.h"

#include <QtTest/QSignalSpy>

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

#include <QtTest/QTest>

void InitialConnectPeripheralStartupTest::init()
{
    VehicleTestManualConnect::init();
    LinkManager::instance()->setConnectionsAllowed();
    MAVLinkProtocol::deleteTempLogFiles();
}

void InitialConnectPeripheralStartupTest::_noCameraOrGimbalRequestsBeforeInitialConnectComplete()
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    QSignalSpy activeVehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(activeVehicleSpy.isValid());

    _mockLink = MockLink::startPX4MockLink(false /* sendStatusText */, true /* enableCamera */, true /* enableGimbal */,
                                           MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost);
    QVERIFY(_mockLink);

    QVERIFY_SIGNAL_WAIT(activeVehicleSpy, TestTimeout::longMs());
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);

    QSignalSpy initialConnectCompleteSpy(_vehicle, &Vehicle::initialConnectComplete);
    QVERIFY(initialConnectCompleteSpy.isValid());

    // Test pre-complete behavior only while initial connect is still in progress.
    QVERIFY2(!_vehicle->isInitialConnectComplete(), "Initial connect completed too quickly for pre-complete assertions");

    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->clearReceivedRequestMessageCounts();

    const QDeadlineTimer preCompleteDeadline(TestTimeout::longMs());
    while (!_vehicle->isInitialConnectComplete() && (initialConnectCompleteSpy.count() == 0)) {
        QCOMPARE(_mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION), 0);
        QCOMPARE(_mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_CAMERA_INFORMATION), 0);
        QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_CAMERA_INFORMATION), 0);

        QVERIFY2(!preCompleteDeadline.hasExpired(), "Timed out waiting for initialConnectComplete");
        QTest::qWait(20);
    }

    // After initial connect completes, both managers should begin startup requests.
    QVERIFY_TRUE_WAIT(
        (_mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_CAMERA_INFORMATION) > 0)
            || (_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_CAMERA_INFORMATION) > 0),
        TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(_mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION) > 0,
                      TestTimeout::mediumMs());

    _disconnectMockLink();
}

UT_REGISTER_TEST(InitialConnectPeripheralStartupTest, TestLabel::Integration, TestLabel::Vehicle)
