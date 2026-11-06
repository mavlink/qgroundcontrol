#include "MotorIdentificationControllerTest.h"

#include <QtTest/QSignalSpy>

#include "MAVLinkLib.h"
#include "MockLink.h"
#include "MotorIdentificationController.h"
#include "Vehicle.h"

UT_REGISTER_TEST(MotorIdentificationControllerTest, TestLabel::Integration, TestLabel::Vehicle)

bool MotorIdentificationControllerTest::_lastMotorTestIs(int motor, int throttlePercent, int timeoutSecs) const
{
    mavlink_message_t message{};
    if (!mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_COMMAND_LONG, message)) {
        return false;
    }

    mavlink_command_long_t command{};
    mavlink_msg_command_long_decode(&message, &command);
    return (command.command == MAV_CMD_DO_MOTOR_TEST) && (static_cast<int>(command.param1) == motor) &&
           (static_cast<int>(command.param2) == MOTOR_TEST_THROTTLE_PERCENT) &&
           (static_cast<int>(command.param3) == throttlePercent) && (static_cast<int>(command.param4) == timeoutSecs);
}

void MotorIdentificationControllerTest::_layoutAngles_data()
{
    QTest::addColumn<int>("motorCount");
    QTest::addColumn<bool>("xLayout");
    QTest::addColumn<QList<double>>("expectedAngles");

    QTest::newRow("quadX") << 4 << true << QList<double>{45, 135, 225, 315};
    QTest::newRow("quadPlus") << 4 << false << QList<double>{0, 90, 180, 270};
    QTest::newRow("tri") << 3 << true << QList<double>{60, 180, 300};
    QTest::newRow("hexaX") << 6 << true << QList<double>{30, 90, 150, 210, 270, 330};
    QTest::newRow("octaPlus") << 8 << false << QList<double>{0, 45, 90, 135, 180, 225, 270, 315};
    QTest::newRow("unknown") << -1 << true << QList<double>{};
}

// Positions run clockwise from the nose, which is the order ArduPilot's motor test steps through the motors
void MotorIdentificationControllerTest::_layoutAngles()
{
    QFETCH(int, motorCount);
    QFETCH(bool, xLayout);
    QFETCH(QList<double>, expectedAngles);

    QCOMPARE(MotorIdentificationController::layoutAngles(motorCount, xLayout), expectedAngles);
}

void MotorIdentificationControllerTest::_quadLayout()
{
    MotorIdentificationController controller;

    QVERIFY(controller.supported());
    QCOMPARE(controller.motorCount(), 4);
    QCOMPARE(controller.positionAngles().count(), 4);
    QVERIFY(!controller.running());
    QCOMPARE(controller.currentMotor(), -1);
    QVERIFY(!controller.complete());
}

// Without a known layout the positions are only recorded, never judged or remapped
void MotorIdentificationControllerTest::_identify()
{
    const QList<int> positions{1, 0, 2, 3};
    constexpr int throttlePercent = 10;
    constexpr int pulseDurationSecs = 1;

    MotorIdentificationController controller;
    controller.setPulseIntervalMs(50);
    QVERIFY(!controller.layoutKnown());
    QSignalSpy runningSpy(&controller, &MotorIdentificationController::runningChanged);
    QVERIFY(runningSpy.isValid());

    controller.start(throttlePercent);
    QVERIFY(controller.running());
    QCOMPARE(runningSpy.count(), 1);

    QVariantList expectedSelections;
    for (int motor = 0; motor < positions.count(); motor++) {
        QCOMPARE(controller.currentMotor(), motor);
        // MockLink sees each pulse as a motor test of the current motor, which is numbered from 1
        QCOMPARE(controller.motorTestNumber(motor), motor + 1);
        QVERIFY_TRUE_WAIT(_lastMotorTestIs(motor + 1, throttlePercent, pulseDurationSecs), TestTimeout::mediumMs());

        controller.selectPosition(positions[motor]);
        expectedSelections.append(positions[motor]);
        QCOMPARE(controller.selectedPositions(), expectedSelections);
    }
    QVERIFY_TRUE_WAIT(_lastMotorTestIs(static_cast<int>(positions.count()), 0, 0), TestTimeout::mediumMs());

    QVERIFY(controller.complete());
    QVERIFY(!controller.running());
    QCOMPARE(controller.currentMotor(), -1);
    QCOMPARE(runningSpy.count(), 2);
    QVERIFY(!controller.orderCorrect());
    QVERIFY(controller.remapChanges().isEmpty());
}

void MotorIdentificationControllerTest::_ignoresUnavailablePositions()
{
    MotorIdentificationController controller;
    controller.setPulseIntervalMs(50);
    QSignalSpy selectionsSpy(&controller, &MotorIdentificationController::selectedPositionsChanged);
    QVERIFY(selectionsSpy.isValid());

    controller.selectPosition(0);
    QVERIFY(selectionsSpy.isEmpty());

    controller.start(10);
    selectionsSpy.clear();
    controller.selectPosition(-1);
    controller.selectPosition(controller.motorCount());
    QVERIFY(selectionsSpy.isEmpty());

    controller.selectPosition(2);
    controller.selectPosition(2);
    QCOMPARE(selectionsSpy.count(), 1);
    QCOMPARE(controller.selectedPositions(), QVariantList{2});
    QCOMPARE(controller.currentMotor(), 1);

    controller.cancel();
}

void MotorIdentificationControllerTest::_cancelStopsPulsingMotor()
{
    MotorIdentificationController controller;
    controller.setPulseIntervalMs(50);

    controller.start(10);
    controller.selectPosition(0);
    QVERIFY_TRUE_WAIT(_lastMotorTestIs(2, 10, 1), TestTimeout::mediumMs());

    controller.cancel();
    QVERIFY(!controller.running());
    QVERIFY(controller.selectedPositions().isEmpty());
    QVERIFY_TRUE_WAIT(_lastMotorTestIs(2, 0, 0), TestTimeout::mediumMs());
}

void MotorIdentificationControllerTest::_startRejectsInvalidThrottle()
{
    MotorIdentificationController controller;

    expectLogMessage("AutoPilotPlugins.MotorIdentificationController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid throttle")));
    controller.start(0);
    verifyExpectedLogMessage();

    QVERIFY(!controller.running());
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_DO_MOTOR_TEST), 0);
}
