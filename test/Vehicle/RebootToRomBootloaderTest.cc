#include "RebootToRomBootloaderTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(RebootToRomBootloaderTest, TestLabel::Integration, TestLabel::Vehicle)

void RebootToRomBootloaderTest::_connect(MAV_AUTOPILOT autopilot, MAV_RESULT romBootloaderResult)
{
    if ((autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    _connectMockLink(autopilot);
    if (QTest::currentTestFailed()) {
        return;
    }
    mockLink()->setRebootToRomBootloaderResult(romBootloaderResult);
}

void RebootToRomBootloaderTest::_waitForRebootAcked()
{
    QVERIFY_TRUE_WAIT(
        !vehicle()->isMavCommandPending(vehicle()->defaultComponentId(), MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN),
        TestTimeout::mediumMs());
}

void RebootToRomBootloaderTest::_flushCommandQueue()
{
    // Send a tracked no-op so any further reboot command would already have reached MockLink
    mockLink()->clearReceivedMavCommandCounts();
    vehicle()->sendMavCommand(vehicle()->defaultComponentId(), MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED,
                              false /* showError */);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED) == 1,
                      TestTimeout::mediumMs());
}

void RebootToRomBootloaderTest::_verifyRebootParams(const QSignalSpy& spy, int index, float param1, float param2,
                                                    float param3, float param4)
{
    QVERIFY(index < spy.count());
    const QList<QVariant>& args = spy.at(index);
    QCOMPARE(args.at(0).toFloat(), param1);
    QCOMPARE(args.at(1).toFloat(), param2);
    QCOMPARE(args.at(2).toFloat(), param3);
    QCOMPARE(args.at(3).toFloat(), param4);
}

void RebootToRomBootloaderTest::_apmUnsupportedFallsBackToDfuSequence()
{
    _connect(MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_RESULT_UNSUPPORTED);
    if (QTest::currentTestFailed()) {
        return;
    }
    QSignalSpy spyReboot(mockLink(), &MockLink::rebootShutdownCommandReceived);
    QVERIFY(spyReboot.isValid());

    vehicle()->rebootVehicleToRomBootloader();

    // MockLink accepts the fallback sequence, which closes the vehicle
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->activeVehicle() == nullptr, TestTimeout::longMs());
    QCOMPARE(spyReboot.count(), 2);
    _verifyRebootParams(spyReboot, 0, Vehicle::kRebootShutdownActionRebootToRomBootloader);
    _verifyRebootParams(spyReboot, 1, 42.0f, 24.0f, 71.0f, 99.0f);
}

void RebootToRomBootloaderTest::_apmAcceptedDoesNotFallBack()
{
    _connect(MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_RESULT_ACCEPTED);
    if (QTest::currentTestFailed()) {
        return;
    }
    QSignalSpy spyReboot(mockLink(), &MockLink::rebootShutdownCommandReceived);
    QVERIFY(spyReboot.isValid());

    vehicle()->rebootVehicleToRomBootloader();

    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->activeVehicle() == nullptr, TestTimeout::longMs());
    QCOMPARE(spyReboot.count(), 1);
    _verifyRebootParams(spyReboot, 0, Vehicle::kRebootShutdownActionRebootToRomBootloader);
}

void RebootToRomBootloaderTest::_apmFailedIsReported()
{
    _connect(MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_RESULT_FAILED);
    if (QTest::currentTestFailed()) {
        return;
    }
    QSignalSpy spyReboot(mockLink(), &MockLink::rebootShutdownCommandReceived);
    QVERIFY(spyReboot.isValid());

    expectAppMessage(QRegularExpression(QStringLiteral("refused to reboot into the ROM bootloader")));
    vehicle()->rebootVehicleToRomBootloader();
    _waitForRebootAcked();
    verifyExpectedLogMessage();

    _flushCommandQueue();
    QCOMPARE(spyReboot.count(), 1);
    _verifyRebootParams(spyReboot, 0, Vehicle::kRebootShutdownActionRebootToRomBootloader);
    QVERIFY(MultiVehicleManager::instance()->activeVehicle());
}

void RebootToRomBootloaderTest::_px4UnsupportedIsReported()
{
    _connect(MAV_AUTOPILOT_PX4, MAV_RESULT_UNSUPPORTED);
    if (QTest::currentTestFailed()) {
        return;
    }
    QSignalSpy spyReboot(mockLink(), &MockLink::rebootShutdownCommandReceived);
    QVERIFY(spyReboot.isValid());

    expectAppMessage(QRegularExpression(QStringLiteral("does not support rebooting into the ROM bootloader")));
    vehicle()->rebootVehicleToRomBootloader();
    _waitForRebootAcked();
    verifyExpectedLogMessage();

    // The ArduPilot DFU sequence is never sent to other firmware
    _flushCommandQueue();
    QCOMPARE(spyReboot.count(), 1);
    _verifyRebootParams(spyReboot, 0, Vehicle::kRebootShutdownActionRebootToRomBootloader);
}

void RebootToRomBootloaderTest::_armedSendsNothing()
{
    _connect(MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_RESULT_ACCEPTED);
    if (QTest::currentTestFailed()) {
        return;
    }
    QSignalSpy spyReboot(mockLink(), &MockLink::rebootShutdownCommandReceived);
    QVERIFY(spyReboot.isValid());

    mockLink()->setArmed(true);
    QVERIFY_TRUE_WAIT(vehicle()->armed(), TestTimeout::mediumMs());

    expectAppMessage(QRegularExpression(QStringLiteral("Disarm the vehicle")));
    vehicle()->rebootVehicleToRomBootloader();
    verifyExpectedLogMessage();

    _flushCommandQueue();
    QCOMPARE(spyReboot.count(), 0);
}
