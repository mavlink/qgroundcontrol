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
}

void InitialConnectPeripheralStartupTest::_noCameraOrGimbalRequestsBeforeInitialConnectComplete()
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    QSignalSpy activeVehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(activeVehicleSpy.isValid());

    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionEnableCamera | MockConfiguration::OptionEnableGimbal);
    QVERIFY(_mockLink);

    QVERIFY_SIGNAL_WAIT(activeVehicleSpy, TestTimeout::longMs());
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);

    // Test pre-complete behavior only while initial connect is still in progress.
    QVERIFY2(!_vehicle->isInitialConnectComplete(), "Initial connect completed too quickly for pre-complete assertions");

    // Received-message counts are monotonic, so counts that are still zero when initialConnectComplete
    // fires prove no peripheral request was sent before completion. They are sampled inside the signal
    // handler because the managers send their first request on the next peripheral message after it.
    int gimbalInfoRequestsAtComplete = -1;
    int cameraInfoRequestsAtComplete = -1;
    int cameraInfoCommandsAtComplete = -1;
    (void) connect(_vehicle, &Vehicle::initialConnectComplete, this, [&]() {
        gimbalInfoRequestsAtComplete = _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION);
        cameraInfoRequestsAtComplete = _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_CAMERA_INFORMATION);
        cameraInfoCommandsAtComplete = _mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_CAMERA_INFORMATION);
    });
    QVERIFY_TRUE_WAIT(_vehicle->isInitialConnectComplete(), TestTimeout::longMs());
    QCOMPARE(gimbalInfoRequestsAtComplete, 0);
    QCOMPARE(cameraInfoRequestsAtComplete, 0);
    QCOMPARE(cameraInfoCommandsAtComplete, 0);

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
