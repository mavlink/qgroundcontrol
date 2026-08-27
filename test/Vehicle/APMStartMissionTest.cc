#include "APMStartMissionTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "APMFirmwarePlugin.h"
#include "FirmwarePlugin.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(APMStartMissionTest, TestLabel::Integration, TestLabel::Vehicle)

void APMStartMissionTest::init()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    VehicleTestManualConnect::init();
}

void APMStartMissionTest::_connectAPMVehicle(bool plane)
{
    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

    _mockLink = plane ? MockLink::startAPMArduPlaneMockLink() : MockLink::startAPMArduCopterMockLink();
    QVERIFY2(_mockLink, "Failed to start MockLink");
    (void) connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });

    QVERIFY2(UnitTest::waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")),
             "Timeout waiting for vehicle connection");
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY2(_vehicle, "Vehicle should not be null after connection");

    QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
    QVERIFY2(spyConnect.isValid(), "Failed to create spy for initialConnectComplete");
    QVERIFY2(UnitTest::waitForSignal(spyConnect, TestTimeout::longMs(), QStringLiteral("initialConnectComplete")),
             "Timeout waiting for initial connect");

    mockLink()->clearReceivedMavCommandCounts();
}

void APMStartMissionTest::_setArmedOnGround()
{
    mockLink()->setArmed(true);
    QVERIFY_TRUE_WAIT(vehicle()->armed(), TestTimeout::mediumMs());
    QVERIFY(!vehicle()->flying());
}

void APMStartMissionTest::_verifyNoMissionStartSent()
{
    // Flush the command queue with a tracked no-op so any MISSION_START would already be counted
    vehicle()->sendMavCommand(vehicle()->defaultComponentId(), MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED, false /* showError */);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED) == 1,
                      TestTimeout::mediumMs());
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_MISSION_START), 0);
}

void APMStartMissionTest::_flyingCopterSwitchesToAuto()
{
    _connectAPMVehicle(false /* plane */);
    if (QTest::currentTestFailed()) {
        return;
    }

    // Takeoff raises the mock altitude, which drives both flying signals (HEARTBEAT
    // MAV_STATE_ACTIVE for ArduPilot and EXTENDED_SYS_STATE IN_AIR).
    // The flying transition creates QGCPressure, which warns on hosts without a pressure backend.
    ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
    ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));
    mockLink()->setArmed(true);
    vehicle()->sendMavCommand(vehicle()->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */,
                              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f /* altitude */);
    QVERIFY_TRUE_WAIT(vehicle()->flying(), TestTimeout::mediumMs());

    vehicle()->startMission();

    QCOMPARE(vehicle()->flightMode(), vehicle()->firmwarePlugin()->missionFlightMode());
    _verifyNoMissionStartSent();
}

void APMStartMissionTest::_disarmedCopterArmsInGuidedThenMissionStart()
{
    _connectAPMVehicle(false /* plane */);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(!vehicle()->armed());
    vehicle()->startMission();

    const APMFirmwarePlugin* apmPlugin = qobject_cast<const APMFirmwarePlugin*>(vehicle()->firmwarePlugin());
    QVERIFY(apmPlugin);
    QCOMPARE(vehicle()->flightMode(), apmPlugin->guidedFlightMode());
    QVERIFY(vehicle()->armed());
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_MISSION_START) == 1, TestTimeout::mediumMs());
}

void APMStartMissionTest::_armedCopterSendsMissionStart()
{
    _connectAPMVehicle(false /* plane */);
    if (QTest::currentTestFailed()) {
        return;
    }

    _setArmedOnGround();
    const QString flightModeBefore = vehicle()->flightMode();

    vehicle()->startMission();

    // Armed non-fixed-wing must start the mission via MISSION_START, not a bare mode change
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_MISSION_START) == 1, TestTimeout::mediumMs());
    QCOMPARE(vehicle()->flightMode(), flightModeBefore);
}

void APMStartMissionTest::_disarmedPlaneSwitchesToAutoThenArms()
{
    _connectAPMVehicle(true /* plane */);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(!vehicle()->armed());
    vehicle()->startMission();

    QCOMPARE(vehicle()->flightMode(), vehicle()->firmwarePlugin()->missionFlightMode());
    QVERIFY(vehicle()->armed());
    _verifyNoMissionStartSent();
}

void APMStartMissionTest::_armedPlaneSwitchesToAuto()
{
    _connectAPMVehicle(true /* plane */);
    if (QTest::currentTestFailed()) {
        return;
    }

    _setArmedOnGround();

    vehicle()->startMission();

    // An armed fixed wing on the ground must be switched to Auto to start the mission
    QCOMPARE(vehicle()->flightMode(), vehicle()->firmwarePlugin()->missionFlightMode());
    _verifyNoMissionStartSent();
}
