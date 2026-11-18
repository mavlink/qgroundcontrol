#include "AdditionalStatesCoverageTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>

#include "MAVLinkLib.h"
#include "SendMavlinkCommandState.h"
#include "SendMavlinkMessageState.h"
#include "StateTestCommon.h"
#include "WaitForMavlinkMessageState.h"
#include "WaitForParamResponseState.h"

void AdditionalStatesCoverageTest::_testStatesConstructWithoutVehicle()
{
    // These states act on a vehicle once a machine enters them. Without one, this constructs each with its
    // optional arguments and destroys it with the machine.
    QStateMachine machine;

    new SendMavlinkCommandState(&machine, MAV_CMD_COMPONENT_ARM_DISARM, 1.0);
    auto* deferredCommandState = new SendMavlinkCommandState(&machine);
    deferredCommandState->setup(MAV_CMD_DO_SET_MODE, 1.0, 4.0);
    new SendMavlinkMessageState(&machine, [](quint32, uint8_t, mavlink_message_t*) {}, 3);
    new WaitForMavlinkMessageState(&machine, MAVLINK_MSG_ID_HEARTBEAT, 5000,
                                   [](const mavlink_message_t&) { return true; });

    QCOMPARE(machine.findChildren<QGCState*>().size(), 4);
}

void AdditionalStatesCoverageTest::_testVehicleStatesFailWithoutVehicle_data()
{
    using StateFactory = std::function<QGCState*(QState*)>;
    QTest::addColumn<StateFactory>("createState");

    QTest::newRow("SendMavlinkCommandState") << StateFactory(
        [](QState* parent) { return new SendMavlinkCommandState(parent, MAV_CMD_COMPONENT_ARM_DISARM); });
    QTest::newRow("SendMavlinkMessageState") << StateFactory([](QState* parent) {
        return new SendMavlinkMessageState(parent, [](quint32, uint8_t, mavlink_message_t*) {}, 0);
    });
    QTest::newRow("WaitForMavlinkMessageState") << StateFactory(
        [](QState* parent) { return new WaitForMavlinkMessageState(parent, MAVLINK_MSG_ID_HEARTBEAT, 0); });
    QTest::newRow("WaitForParamResponseState")
        << StateFactory([](QState* parent) { return new WaitForParamResponseState(parent, 0, nullptr, nullptr); });
}

void AdditionalStatesCoverageTest::_testVehicleStatesFailWithoutVehicle()
{
    using StateFactory = std::function<QGCState*(QState*)>;
    QFETCH(StateFactory, createState);

    QGCStateMachine machine(QStringLiteral("TestMachine"), nullptr);
    QGCState* const state = createState(&machine);
    auto* finalState = addFinalState(&machine);

    state->addTransition(state, &QGCState::error, finalState);
    machine.setInitialState(state);

    QSignalSpy errorSpy(state, &QGCState::error);
    expectLogMessage("Utilities.QGCStateMachine", QtWarningMsg, QRegularExpression("No vehicle"));
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QCOMPARE(errorSpy.count(), 1);
}

void AdditionalStatesCoverageTest::_testDelayStateMachineStopEndsDelay_data()
{
    QTest::addColumn<bool>("restart");

    QTest::newRow("stop()") << false;
    QTest::newRow("restart()") << true;
}

void AdditionalStatesCoverageTest::_testDelayStateMachineStopEndsDelay()
{
    QFETCH(bool, restart);

    QGCStateMachine machine(QStringLiteral("StopEndsDelayTest"), nullptr);
    const int delayMs = 200;

    // Not the initial state, so that the run after a restart does not enter it again
    auto* firstState = new QGCState(QStringLiteral("First"), &machine);
    auto* delayState = new DelayState(&machine, delayMs);
    machine.addEventTransition(firstState, QStringLiteral("next"), delayState);
    machine.setInitialState(firstState);

    QSignalSpy firstEnteredSpy(firstState, &QState::entered);
    QSignalSpy delayEnteredSpy(delayState, &QState::entered);
    QSignalSpy delayExitedSpy(delayState, &QState::exited);
    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);
    QSignalSpy delayCompleteSpy(delayState, &DelayState::delayComplete);
    QSignalSpy advanceSpy(delayState, &QGCState::advance);

    QVERIFY(startAndWaitForEntered(&machine, firstState));
    machine.postEvent(QStringLiteral("next"));
    QVERIFY(spyTriggered(delayEnteredSpy));

    if (restart) {
        machine.restart();
        QTRY_COMPARE_WITH_TIMEOUT(firstEnteredSpy.count(), 2, TestTimeout::shortMs());
    } else {
        machine.stop();
        QVERIFY(spyTriggered(stoppedSpy));
    }

    // Qt has not exited the state, and still the delay that was running does not end
    QCOMPARE(delayExitedSpy.count(), 0);
    QVERIFY_NO_SIGNAL_WAIT(delayCompleteSpy, delayMs * 2);
    QCOMPARE(advanceSpy.count(), 0);
}

void AdditionalStatesCoverageTest::_testDelayStateRestartStartsDelayOver()
{
    QGCStateMachine machine(QStringLiteral("RestartDelayTest"), nullptr);
    const int delayMs = 200;

    // The initial state, so that it is active when the machine restarts and the next run enters it again
    auto* delayState = new DelayState(&machine, delayMs);
    delayState->addTransition(delayState, &DelayState::delayComplete, addFinalState(&machine));
    machine.setInitialState(delayState);

    // The time since the state was last entered, taken when the delay ends
    QElapsedTimer sinceEntered;
    qint64 delayTakenMs = -1;
    connect(delayState, &QState::entered, delayState, [&sinceEntered]() { sinceEntered.start(); });
    connect(delayState, &DelayState::delayComplete, delayState,
            [&sinceEntered, &delayTakenMs]() { delayTakenMs = sinceEntered.elapsed(); });

    QSignalSpy enteredSpy(delayState, &QState::entered);
    QSignalSpy delayCompleteSpy(delayState, &DelayState::delayComplete);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    // Restart when a part of the delay has passed
    QVERIFY(startAndWaitForEntered(&machine, delayState));
    QVERIFY_NO_SIGNAL_WAIT(delayCompleteSpy, delayMs * 3 / 10);
    machine.restart();

    // One delay ends, and it is a whole one counted from the entry of the second run
    QVERIFY(spyTriggered(finishedSpy));
    QCOMPARE(enteredSpy.count(), 2);
    QCOMPARE(delayCompleteSpy.count(), 1);
    QVERIFY2(delayTakenMs >= delayMs - delayMs / 10, qPrintable(QString::number(delayTakenMs)));
}

UT_REGISTER_TEST(AdditionalStatesCoverageTest, TestLabel::Unit, TestLabel::Utilities)
