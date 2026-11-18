#include "AdditionalStatesCoverageTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>

#include "MAVLinkLib.h"
#include "SendMavlinkCommandState.h"
#include "SendMavlinkMessageState.h"
#include "StateTestCommon.h"
#include "Vehicle.h"
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
    finishOn(machine, state, &QGCState::error);

    QSignalSpy errorSpy(state, &QGCState::error);
    expectLogMessage("Utilities.QGCStateMachine", QtWarningMsg, QRegularExpression("No vehicle"));
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QCOMPARE(errorSpy.count(), 1);
}

void AdditionalStatesCoverageTest::_testFunctionState()
{
    QStateMachine machine;
    bool functionCalled = false;

    auto* state =
        new FunctionState(QStringLiteral("TestFunction"), &machine, [&functionCalled]() { functionCalled = true; });

    QVERIFY(runStateToCompletion(state, &machine));
    QVERIFY(functionCalled);
}

void AdditionalStatesCoverageTest::_testFunctionStateChain()
{
    QStateMachine machine;
    QStringList executionOrder;

    auto* state1 = new FunctionState(QStringLiteral("State1"), &machine,
                                     [&executionOrder]() { executionOrder.append(QStringLiteral("state1")); });
    auto* state2 = new FunctionState(QStringLiteral("State2"), &machine,
                                     [&executionOrder]() { executionOrder.append(QStringLiteral("state2")); });
    auto* state3 = new FunctionState(QStringLiteral("State3"), &machine,
                                     [&executionOrder]() { executionOrder.append(QStringLiteral("state3")); });
    auto* finalState = addFinalState(&machine);

    state1->addTransition(state1, &QGCState::advance, state2);
    state2->addTransition(state2, &QGCState::advance, state3);
    state3->addTransition(state3, &QGCState::advance, finalState);
    machine.setInitialState(state1);

    QVERIFY(startAndWaitForFinished(&machine));
    QCOMPARE(executionOrder.size(), 3);
    QCOMPARE(executionOrder[0], QStringLiteral("state1"));
    QCOMPARE(executionOrder[1], QStringLiteral("state2"));
    QCOMPARE(executionOrder[2], QStringLiteral("state3"));
}

void AdditionalStatesCoverageTest::_testDelayState()
{
    QStateMachine machine;
    const int delayMs = 100;

    auto* delayState = new DelayState(&machine, delayMs);
    finishOn(machine, delayState, &DelayState::delayComplete);

    QElapsedTimer timer;
    timer.start();

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(timer.elapsed() >= delayMs - 10);
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

void AdditionalStatesCoverageTest::_testQGCStateEntryExitCallbacks()
{
    QStateMachine machine;
    bool entryCalled = false;
    bool exitCalled = false;

    auto* state = new QGCState(QStringLiteral("TestState"), &machine);
    state->setOnEntry([&entryCalled]() { entryCalled = true; });
    state->setOnExit([&exitCalled]() { exitCalled = true; });

    finishOn(machine, state, &QState::entered);

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(entryCalled);
    QVERIFY(exitCalled);
}

void AdditionalStatesCoverageTest::_testQGCStateOnEnterOnLeaveVirtuals()
{
    // Test that onEnter/onLeave virtuals are called in the correct order
    class TestState : public QGCState
    {
    public:
        TestState(QState* parent)
            : QGCState(QStringLiteral("TestState"), parent)
        {}

        bool onEnterCalled = false;
        bool onLeaveCalled = false;

    protected:
        void onEnter() override { onEnterCalled = true; }

        void onLeave() override { onLeaveCalled = true; }
    };

    QStateMachine machine;
    auto* state = new TestState(&machine);
    finishOn(machine, state, &QState::entered);

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(state->onEnterCalled);
    QVERIFY(state->onLeaveCalled);
}

void AdditionalStatesCoverageTest::_testQGCFinalState()
{
    QGCStateMachine machine(QStringLiteral("FinalStateTest"), nullptr);

    auto* startState = new FunctionState(QStringLiteral("Start"), &machine, []() {});
    auto* finalState = new QGCFinalState(QStringLiteral("MyFinal"), &machine);

    startState->addTransition(startState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QCOMPARE(finalState->objectName(), QStringLiteral("MyFinal"));

    QVERIFY(startAndWaitForFinished(&machine));
}

void AdditionalStatesCoverageTest::_testWaitForMavlinkMessageCompletesOnFirstMatch_data()
{
    QTest::addColumn<bool>("usePredicate");

    QTest::newRow("any message with the id") << false;
    QTest::newRow("message the predicate accepts") << true;
}

void AdditionalStatesCoverageTest::_testWaitForMavlinkMessageCompletesOnFirstMatch()
{
    QFETCH(bool, usePredicate);

    // An offline vehicle, whose message signal the test emits
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    QGCStateMachine machine(QStringLiteral("TestMachine"), &vehicle);

    constexpr uint8_t acceptedSystemId = 7;
    int predicateCallCount = 0;
    WaitForMavlinkMessageState::Predicate predicate;
    if (usePredicate) {
        predicate = [&predicateCallCount](const mavlink_message_t& message) {
            predicateCallCount++;
            return message.sysid == acceptedSystemId;
        };
    }
    auto* const state = new WaitForMavlinkMessageState(&machine, MAVLINK_MSG_ID_DEBUG, 0, predicate);
    state->addTransition(state, &QGCState::advance, addFinalState(&machine));
    machine.setInitialState(state);

    QSignalSpy completedSpy(state, &WaitStateBase::completed);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(startAndWaitForEntered(&machine, state));

    mavlink_message_t message{};
    message.sysid = acceptedSystemId;
    message.msgid = MAVLINK_MSG_ID_DEBUG_VECT;
    emit vehicle.mavlinkMessageReceived(message);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(predicateCallCount, 0);

    message.msgid = MAVLINK_MSG_ID_DEBUG;
    if (usePredicate) {
        message.sysid = acceptedSystemId + 1;
        emit vehicle.mavlinkMessageReceived(message);
        QCOMPARE(completedSpy.count(), 0);
        QCOMPARE(predicateCallCount, 1);
        message.sysid = acceptedSystemId;
    }

    emit vehicle.mavlinkMessageReceived(message);
    QCOMPARE(completedSpy.count(), 1);
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::shortMs());

    // The state has stopped listening
    const int predicateCallsWhenCompleted = predicateCallCount;
    emit vehicle.mavlinkMessageReceived(message);
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(predicateCallCount, predicateCallsWhenCompleted);
}

UT_REGISTER_TEST(AdditionalStatesCoverageTest, TestLabel::Unit, TestLabel::Utilities)
