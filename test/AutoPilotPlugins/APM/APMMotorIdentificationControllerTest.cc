#include "APMMotorIdentificationControllerTest.h"

#include <cmath>

#include <QtTest/QSignalSpy>

#include "APMMotorIdentificationController.h"
#include "Fact.h"
#include "MockLink.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "Vehicle.h"

UT_REGISTER_TEST(APMMotorIdentificationControllerTest, TestLabel::Integration, TestLabel::Vehicle)

// SERVOn_FUNCTION value keyed by n. Named so QFETCH can take it.
using APMMotorIdServoFunctions = QMap<int, int>;

void APMMotorIdentificationControllerTest::_setFrame(int frameClass, int frameType)
{
    _setParam(QStringLiteral("FRAME_CLASS"), frameClass);
    _setParam(QStringLiteral("FRAME_TYPE"), frameType);
}

void APMMotorIdentificationControllerTest::_setParam(const QString& name, int value)
{
    Fact* const fact = vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId, name);
    // An unchanged value isn't sent to the vehicle, so there would be no ack to wait for
    if (fact->rawValue().toInt() == value) {
        return;
    }

    QSignalSpy ackSpy(fact, &Fact::vehicleUpdated);
    QVERIFY(ackSpy.isValid());
    // ArduPilot only applies a new frame after a reboot
    qgcApp()->resetRebootMessageDebounce();
    expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required")));
    fact->setRawValue(value);
    QVERIFY(ackSpy.wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
}

int APMMotorIdentificationControllerTest::_intParam(const QString& name) const
{
    return vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId, name)->rawValue().toInt();
}

void APMMotorIdentificationControllerTest::_loadLayout_data()
{
    QTest::addColumn<int>("frameClass");
    QTest::addColumn<int>("frameType");
    QTest::addColumn<bool>("hasLayout");
    QTest::addColumn<QList<int>>("motorNumbers");
    QTest::addColumn<QList<int>>("testOrders");
    QTest::addColumn<QList<double>>("angles");

    QTest::newRow("quadX") << 1 << 1 << true << QList<int>{1, 4, 2, 3} << QList<int>{1, 2, 3, 4}
                           << QList<double>{45, 135, 225, 315};
    QTest::newRow("quadPlus") << 1 << 0 << true << QList<int>{3, 1, 4, 2} << QList<int>{1, 2, 3, 4}
                              << QList<double>{0, 90, 180, 270};
    QTest::newRow("quadBetaflightX") << 1 << 12 << true << QList<int>{2, 1, 3, 4} << QList<int>{1, 2, 3, 4}
                                     << QList<double>{45, 135, 225, 315};
    // The tail servo is test order 3, between the rear and front-left motors
    QTest::newRow("tri") << 7 << 0 << true << QList<int>{1, 4, 2} << QList<int>{1, 2, 4}
                         << QList<double>{63.4, 180, 296.6};
    QTest::newRow("octaQuadStacked") << 4 << 1 << false << QList<int>{} << QList<int>{} << QList<double>{};
    QTest::newRow("noFrame") << 0 << 0 << false << QList<int>{} << QList<int>{} << QList<double>{};
}

void APMMotorIdentificationControllerTest::_loadLayout()
{
    QFETCH(int, frameClass);
    QFETCH(int, frameType);
    QFETCH(bool, hasLayout);
    QFETCH(QList<int>, motorNumbers);
    QFETCH(QList<int>, testOrders);
    QFETCH(QList<double>, angles);

    const std::optional<APMMotorIdentificationController::Layout> layout =
        APMMotorIdentificationController::loadLayout(frameClass, frameType);
    QCOMPARE(layout.has_value(), hasLayout);
    if (!layout) {
        return;
    }

    QCOMPARE(layout->motorNumbers, motorNumbers);
    QCOMPARE(layout->testOrders, testOrders);
    QCOMPARE(layout->angles.count(), angles.count());
    for (int i = 0; i < angles.count(); i++) {
        QVERIFY2(std::abs(layout->angles[i] - angles[i]) < 0.1,
                 qPrintable(QStringLiteral("angle %1: %2").arg(i).arg(layout->angles[i])));
    }
}

void APMMotorIdentificationControllerTest::_motorFunction()
{
    QCOMPARE(APMMotorIdentificationController::motorFunction(1), 33);
    QCOMPARE(APMMotorIdentificationController::motorFunction(8), 40);
    QCOMPARE(APMMotorIdentificationController::motorFunction(9), 82);
    QCOMPARE(APMMotorIdentificationController::motorFunction(12), 85);
    QCOMPARE(APMMotorIdentificationController::motorFunction(13), -1);
}

void APMMotorIdentificationControllerTest::_remapServoFunctions_data()
{
    QTest::addColumn<QList<int>>("selectedPositions");
    QTest::addColumn<QMap<int, int>>("expectedFunctions");

    // Motor N of a quad X is test order A-D: Motor1, Motor4, Motor2, Motor3
    QTest::newRow("correctOrder") << QList<int>{0, 1, 2, 3} << QMap<int, int>{};
    QTest::newRow("frontAndRearRightSwapped") << QList<int>{1, 0, 2, 3} << QMap<int, int>{{1, 36}, {4, 33}};
    QTest::newRow("rotatedOnePosition") << QList<int>{1, 2, 3, 0} << QMap<int, int>{{1, 36}, {2, 35}, {3, 33}, {4, 34}};
}

// Each output takes the function of the motor which belongs at the position its motor spun at
void APMMotorIdentificationControllerTest::_remapServoFunctions()
{
    QFETCH(QList<int>, selectedPositions);
    QFETCH(APMMotorIdServoFunctions, expectedFunctions);

    const QList<int> quadXMotorNumbers{1, 4, 2, 3};
    const QMap<int, int> servoFunctions{{1, 33}, {2, 34}, {3, 35}, {4, 36}, {5, 0}, {9, 56}};

    QCOMPARE(
        APMMotorIdentificationController::remapServoFunctions(quadXMotorNumbers, selectedPositions, servoFunctions),
        expectedFunctions);
}

void APMMotorIdentificationControllerTest::_xConfigFollowsFrameType_data()
{
    QTest::addColumn<int>("frameType");
    QTest::addColumn<bool>("xConfig");

    QTest::newRow("plus") << 0 << false;
    QTest::newRow("x") << 1 << true;
    QTest::newRow("betaflightX") << 12 << true;
}

// ArduCopter 3.5+ reports X or plus through FRAME_TYPE, where 0 is plus
void APMMotorIdentificationControllerTest::_xConfigFollowsFrameType()
{
    QFETCH(int, frameType);
    QFETCH(bool, xConfig);

    _setFrame(1, frameType);
    QCOMPARE(vehicle()->xConfigMotors(), xConfig);
}

// The even spacing could show motors where they aren't, so a frame without a layout hides identification
void APMMotorIdentificationControllerTest::_noLayoutWithoutFrame()
{
    _setFrame(0, 0);

    APMMotorIdentificationController controller;
    QVERIFY(!controller.supported());
    QVERIFY(!controller.layoutKnown());
}

void APMMotorIdentificationControllerTest::_triSkipsTailServo()
{
    _setFrame(7, 0);

    APMMotorIdentificationController controller;
    QVERIFY(controller.supported());
    QCOMPARE(controller.motorCount(), 3);
    QCOMPARE(controller.motorTestNumber(0), 1);
    QCOMPARE(controller.motorTestNumber(1), 2);
    QCOMPARE(controller.motorTestNumber(2), 4);
}

void APMMotorIdentificationControllerTest::_remapMotors()
{
    _setFrame(1, 1);
    QCOMPARE(_intParam(QStringLiteral("SERVO1_FUNCTION")), 33);
    QCOMPARE(_intParam(QStringLiteral("SERVO4_FUNCTION")), 36);

    APMMotorIdentificationController controller;
    controller.setPulseIntervalMs(50);
    QVERIFY(controller.supported());
    QVERIFY(controller.layoutKnown());
    QCOMPARE(controller.motorCount(), 4);

    // Motors A and B are wired to each other's ESC
    const QList<int> positions{1, 0, 2, 3};
    controller.start(10);
    for (int motor = 0; motor < positions.count(); motor++) {
        QCOMPARE(controller.currentMotor(), motor);
        controller.selectPosition(positions[motor]);
    }
    QVERIFY(controller.complete());
    QVERIFY(!controller.orderCorrect());
    QCOMPARE(controller.remapChanges(), (QStringList{QStringLiteral("SERVO1_FUNCTION: Motor 1 to Motor 4"),
                                                     QStringLiteral("SERVO4_FUNCTION: Motor 4 to Motor 1")}));

    ParameterManager* const parameterManager = vehicle()->parameterManager();
    QSignalSpy servo1Spy(
        parameterManager->getParameter(ParameterManager::defaultComponentId, QStringLiteral("SERVO1_FUNCTION")),
        &Fact::vehicleUpdated);
    QSignalSpy servo4Spy(
        parameterManager->getParameter(ParameterManager::defaultComponentId, QStringLiteral("SERVO4_FUNCTION")),
        &Fact::vehicleUpdated);
    controller.remapMotors();
    QVERIFY_TRUE_WAIT((servo1Spy.count() > 0) && (servo4Spy.count() > 0), TestTimeout::mediumMs());
    QCOMPARE(_intParam(QStringLiteral("SERVO1_FUNCTION")), 36);
    QCOMPARE(_intParam(QStringLiteral("SERVO2_FUNCTION")), 34);
    QCOMPARE(_intParam(QStringLiteral("SERVO3_FUNCTION")), 35);
    QCOMPARE(_intParam(QStringLiteral("SERVO4_FUNCTION")), 33);
    QVERIFY(!controller.complete());
    QVERIFY(controller.remapChanges().isEmpty());
}
