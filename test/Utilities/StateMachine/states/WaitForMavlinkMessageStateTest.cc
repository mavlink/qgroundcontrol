#include "WaitForMavlinkMessageStateTest.h"

#include "MAVLinkLib.h"
#include "StateTestCommon.h"
#include "Vehicle.h"
#include "WaitForMavlinkMessageState.h"

void WaitForMavlinkMessageStateTest::_testCompletesOnFirstMatchingMessage_data()
{
    QTest::addColumn<bool>("usePredicate");

    QTest::newRow("any message with the id") << false;
    QTest::newRow("message the predicate accepts") << true;
}

void WaitForMavlinkMessageStateTest::_testCompletesOnFirstMatchingMessage()
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

UT_REGISTER_TEST(WaitForMavlinkMessageStateTest, TestLabel::Unit, TestLabel::Utilities)
