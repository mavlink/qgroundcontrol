#include "AdditionalStatesCoverageTest.h"

#include <chrono>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>

#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "SendMavlinkCommandState.h"
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
    new WaitForMavlinkMessageState(&machine, MAVLINK_MSG_ID_HEARTBEAT, 5000,
                                   [](const mavlink_message_t&) { return true; });

    QCOMPARE(machine.findChildren<QGCState*>().size(), 3);
}

void AdditionalStatesCoverageTest::_testVehicleStatesFailWithoutVehicle_data()
{
    using StateFactory = std::function<QGCState*(QState*)>;
    QTest::addColumn<StateFactory>("createState");

    QTest::newRow("SendMavlinkCommandState") << StateFactory(
        [](QState* parent) { return new SendMavlinkCommandState(parent, MAV_CMD_COMPONENT_ARM_DISARM); });
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
    expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg, QRegularExpression("No vehicle"));
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
    finishOn(machine, delayState, &QGCState::advance);

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

    ManualScheduler scheduler;
    QGCStateMachine machine(QStringLiteral("StopEndsDelayTest"), nullptr, nullptr, &scheduler);
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
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(delayMs)));
    QCOMPARE(advanceSpy.count(), 0);
}

void AdditionalStatesCoverageTest::_testDelayStateRestartStartsDelayOver()
{
    ManualScheduler scheduler;
    QGCStateMachine machine(QStringLiteral("RestartDelayTest"), nullptr, nullptr, &scheduler);
    const std::chrono::milliseconds delay(200);
    const std::chrono::milliseconds partOfDelay(60);

    // The initial state, so that it is active when the machine restarts and the next run enters it again
    auto* delayState = new DelayState(&machine, static_cast<int>(delay.count()));
    machine.setInitialState(delayState);

    QSignalSpy enteredSpy(delayState, &QState::entered);
    QSignalSpy advanceSpy(delayState, &QGCState::advance);

    // Restart when a part of the delay has passed
    QVERIFY(startAndWaitForEntered(&machine, delayState));
    QVERIFY(scheduler.advanceBy(partOfDelay));
    machine.restart();
    QTRY_COMPARE_WITH_TIMEOUT(enteredSpy.count(), 2, TestTimeout::shortMs());

    // The delay of the first run does not end, and that of the second is a whole one counted from its entry
    QVERIFY(scheduler.advanceBy(delay - partOfDelay));
    QCOMPARE(advanceSpy.count(), 0);
    QVERIFY(scheduler.advanceBy(partOfDelay));
    QCOMPARE(advanceSpy.count(), 1);
}

void AdditionalStatesCoverageTest::_testQGCStateExitCallback()
{
    QStateMachine machine;
    bool exitCalled = false;

    auto* state = new QGCState(QStringLiteral("TestState"), &machine);
    state->setOnExit([&exitCalled]() { exitCalled = true; });

    finishOn(machine, state, &QState::entered);

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(exitCalled);
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

    QSignalSpy advanceSpy(state, &QGCState::advance);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(startAndWaitForEntered(&machine, state));

    mavlink_message_t message{};
    message.sysid = acceptedSystemId;
    message.msgid = MAVLINK_MSG_ID_DEBUG_VECT;
    emit vehicle.mavlinkMessageReceived(message);
    QCOMPARE(advanceSpy.count(), 0);
    QCOMPARE(predicateCallCount, 0);

    message.msgid = MAVLINK_MSG_ID_DEBUG;
    if (usePredicate) {
        message.sysid = acceptedSystemId + 1;
        emit vehicle.mavlinkMessageReceived(message);
        QCOMPARE(advanceSpy.count(), 0);
        QCOMPARE(predicateCallCount, 1);
        message.sysid = acceptedSystemId;
    }

    emit vehicle.mavlinkMessageReceived(message);
    QCOMPARE(advanceSpy.count(), 1);
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::shortMs());

    // The state has stopped listening
    const int predicateCallsWhenCompleted = predicateCallCount;
    emit vehicle.mavlinkMessageReceived(message);
    QCOMPARE(advanceSpy.count(), 1);
    QCOMPARE(predicateCallCount, predicateCallsWhenCompleted);
}

void AdditionalStatesCoverageTest::_testWaitForParamResponseEndsOnItsAnswer_data()
{
    QTest::addColumn<bool>("paramError");

    QTest::newRow("PARAM_VALUE completes the state") << false;
    QTest::newRow("PARAM_ERROR fails the state") << true;
}

void AdditionalStatesCoverageTest::_testWaitForParamResponseEndsOnItsAnswer()
{
    QFETCH(bool, paramError);

    // An offline vehicle, whose message signal the test emits
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    QGCStateMachine machine(QStringLiteral("TestMachine"), &vehicle);

    // Both predicates take the messages of one component for the answer
    constexpr uint8_t answeringComponentId = MAV_COMP_ID_AUTOPILOT1;
    constexpr uint8_t otherComponentId = MAV_COMP_ID_CAMERA;
    const auto fromAnsweringComponent = [](const mavlink_message_t& message) {
        return message.compid == answeringComponentId;
    };
    auto* const state = new WaitForParamResponseState(&machine, 0, fromAnsweringComponent, fromAnsweringComponent);
    QFinalState* const finalState = addFinalState(&machine);
    state->addTransition(state, &QGCState::advance, finalState);
    state->addTransition(state, &QGCState::error, finalState);
    machine.setInitialState(state);

    QSignalSpy advanceSpy(state, &QGCState::advance);
    QSignalSpy errorSpy(state, &QGCState::error);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(startAndWaitForEntered(&machine, state));

    const auto paramMessage = [](bool error, uint8_t componentId) {
        // The pack functions copy the whole field, so the name is given as one
        const char paramId[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN] = "TEST_PARAM";
        static_assert(sizeof(paramId) == MAVLINK_MSG_PARAM_ERROR_FIELD_PARAM_ID_LEN);
        mavlink_message_t message{};
        if (error) {
            (void) mavlink_msg_param_error_pack(1, componentId, &message, 0, 0, paramId, -1, MAV_PARAM_ERROR_READ_ONLY);
        } else {
            (void) mavlink_msg_param_value_pack(1, componentId, &message, paramId, 1.0f, MAV_PARAM_TYPE_REAL32, 1, 0);
        }
        return message;
    };

    // Neither another message nor a PARAM_VALUE or PARAM_ERROR that its predicate rejects ends the wait
    mavlink_message_t otherMessage{};
    otherMessage.compid = answeringComponentId;
    otherMessage.msgid = MAVLINK_MSG_ID_DEBUG;
    emit vehicle.mavlinkMessageReceived(otherMessage);
    emit vehicle.mavlinkMessageReceived(paramMessage(false, otherComponentId));
    emit vehicle.mavlinkMessageReceived(paramMessage(true, otherComponentId));
    QCOMPARE(advanceSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(state->lastParamError(), static_cast<uint8_t>(MAV_PARAM_ERROR_NO_ERROR));

    emit vehicle.mavlinkMessageReceived(paramMessage(paramError, answeringComponentId));
    QCOMPARE(advanceSpy.count(), paramError ? 0 : 1);
    QCOMPARE(errorSpy.count(), paramError ? 1 : 0);
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::shortMs());
    if (paramError) {
        QCOMPARE(state->lastParamError(), static_cast<uint8_t>(MAV_PARAM_ERROR_READ_ONLY));
        QCOMPARE(state->lastParamErrorString(), QStringLiteral("Parameter is read-only"));
    } else {
        QVERIFY(state->lastParamErrorString().isEmpty());
    }
}

void AdditionalStatesCoverageTest::_testWaitForParamResponseForgetsEarlierError()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    QGCStateMachine machine(QStringLiteral("TestMachine"), &vehicle);

    // After a PARAM_ERROR the state is entered once more, as in a machine that runs again
    auto* const state = new WaitForParamResponseState(&machine, 0, nullptr, nullptr);
    uint8_t errorSeenWhenFailed = MAV_PARAM_ERROR_NO_ERROR;
    auto* const again = new FunctionState(QStringLiteral("Again"), &machine, [state, &errorSeenWhenFailed]() {
        errorSeenWhenFailed = state->lastParamError();
    });
    state->addTransition(state, &QGCState::error, again);
    again->addTransition(again, &QGCState::advance, state);
    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QState::entered);
    QVERIFY(startAndWaitForEntered(&machine, state));

    // The pack function copies the whole field, so the name is given as one
    const char paramId[MAVLINK_MSG_PARAM_ERROR_FIELD_PARAM_ID_LEN] = "TEST_PARAM";
    mavlink_message_t message{};
    (void) mavlink_msg_param_error_pack(1, MAV_COMP_ID_AUTOPILOT1, &message, 0, 0, paramId, -1,
                                        MAV_PARAM_ERROR_READ_ONLY);
    emit vehicle.mavlinkMessageReceived(message);
    QTRY_COMPARE_WITH_TIMEOUT(enteredSpy.count(), 2, TestTimeout::shortMs());
    QCOMPARE(errorSeenWhenFailed, static_cast<uint8_t>(MAV_PARAM_ERROR_READ_ONLY));

    // The wait that has begun has no error yet
    QCOMPARE(state->lastParamError(), static_cast<uint8_t>(MAV_PARAM_ERROR_NO_ERROR));
    QVERIFY(state->lastParamErrorString().isEmpty());
}

UT_REGISTER_TEST(AdditionalStatesCoverageTest, TestLabel::Unit, TestLabel::Utilities)
