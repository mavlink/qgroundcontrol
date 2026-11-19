#include "VehicleMessageWaitStateTest.h"

#include <chrono>

#include <QtCore/QRegularExpression>

#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "ManualScheduler.h"
#include "MockLink.h"
#include "StateTestCommon.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "WaitForMavlinkMessageState.h"

void VehicleMessageWaitStateTest::_testRequestReachesVehicle()
{
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);
    const SharedLinkInterfacePtr link = _vehicle->vehicleLinkManager()->primaryLink().lock();
    QVERIFY(link);

    constexpr uint8_t debugIndex = 7;
    quint32 encoderSystemId = 0;
    int encoderChannel = -1;
    const auto encoder = [&](quint32 systemId, uint8_t channel, mavlink_message_t* message) {
        encoderSystemId = systemId;
        encoderChannel = channel;
        (void) mavlink_msg_debug_pack_chan(systemId, MAVLinkProtocol::getComponentId(), channel, message, 0, debugIndex,
                                           0.0f);
    };

    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    auto* const state = new WaitForMavlinkMessageState(&machine, MAVLINK_MSG_ID_DEBUG_VECT, 0);
    state->setRequest(encoder, 0);
    machine.setInitialState(state);

    _mockLink->clearReceivedMavlinkMessageCounts();
    QVERIFY(startAndWaitForEntered(&machine, state));

    // The request is encoded for the primary link of the vehicle
    QCOMPARE(encoderSystemId, MAVLinkProtocol::instance()->getSystemId());
    QCOMPARE(encoderChannel, static_cast<int>(link->mavlinkChannel()));

    QVERIFY_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_DEBUG) == 1, TestTimeout::mediumMs());
    mavlink_message_t received{};
    QVERIFY(_mockLink->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_DEBUG, received));
    QCOMPARE(mavlink_msg_debug_get_ind(&received), debugIndex);
}

void VehicleMessageWaitStateTest::_testRequestIsSentAgainAfterEachTimeout()
{
    ignoreLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Timeout \"")));
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);

    int encoderCallCount = 0;
    const auto encoder = [&encoderCallCount](quint32 systemId, uint8_t channel, mavlink_message_t* message) {
        encoderCallCount++;
        (void) mavlink_msg_debug_pack_chan(systemId, MAVLinkProtocol::getComponentId(), channel, message, 0, 0, 0.0f);
    };

    constexpr int retryCount = 2;
    constexpr int timeoutMs = 200;
    const std::chrono::milliseconds timeout(timeoutMs);
    ManualScheduler scheduler;
    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle, nullptr, &scheduler);

    // Waits for a message that MockLink does not send. Once its attempts are used up the state is entered again,
    // as in a machine that runs again.
    auto* const state = new WaitForMavlinkMessageState(&machine, MAVLINK_MSG_ID_DEBUG_VECT, timeoutMs);
    state->setRequest(encoder, retryCount);
    state->addTransition(state, &WaitStateBase::timedOut, state);
    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QState::entered);
    QSignalSpy timedOutSpy(state, &WaitStateBase::timedOut);
    _mockLink->clearReceivedMavlinkMessageCounts();
    machine.start();
    QVERIFY_SIGNAL_COUNT_WAIT(enteredSpy, 1, TestTimeout::shortMs());
    QCOMPARE(encoderCallCount, 1);

    for (int retry = 1; retry <= retryCount; retry++) {
        QVERIFY(scheduler.advanceBy(timeout));
        QCOMPARE(encoderCallCount, 1 + retry);
        QCOMPARE(timedOutSpy.count(), 0);
    }

    // Only the last attempt ends in timedOut(), which is not followed by another request of that entry
    QVERIFY(scheduler.advanceBy(timeout));
    QCOMPARE(timedOutSpy.count(), 1);
    QVERIFY_SIGNAL_COUNT_WAIT(enteredSpy, 2, TestTimeout::shortMs());
    QCOMPARE(encoderCallCount, retryCount + 2);

    // Entered again, the state has all its retries
    QVERIFY(scheduler.advanceBy(timeout));
    QCOMPARE(encoderCallCount, retryCount + 3);
    QCOMPARE(timedOutSpy.count(), 1);
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_DEBUG) == retryCount + 3,
                      TestTimeout::mediumMs());
}

void VehicleMessageWaitStateTest::_testAnswerToResentRequestCompletes()
{
    ignoreLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Timeout \"")));
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);

    int encoderCallCount = 0;
    const auto encoder = [&encoderCallCount](quint32 systemId, uint8_t channel, mavlink_message_t* message) {
        encoderCallCount++;
        (void) mavlink_msg_debug_pack_chan(systemId, MAVLinkProtocol::getComponentId(), channel, message, 0, 0, 0.0f);
    };

    constexpr int timeoutMs = 200;
    const std::chrono::milliseconds timeout(timeoutMs);
    ManualScheduler scheduler;
    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle, nullptr, &scheduler);
    auto* const state = new WaitForMavlinkMessageState(&machine, MAVLINK_MSG_ID_DEBUG_VECT, timeoutMs);
    state->setRequest(encoder, 2);
    machine.setInitialState(state);

    QSignalSpy advanceSpy(state, &QGCState::advance);
    QSignalSpy timedOutSpy(state, &WaitStateBase::timedOut);
    QVERIFY(startAndWaitForEntered(&machine, state));
    QVERIFY(scheduler.advanceBy(timeout));
    QCOMPARE(encoderCallCount, 2);

    // The state listens again after the timeout, and the answer ends its attempts
    mavlink_message_t answer{};
    answer.msgid = MAVLINK_MSG_ID_DEBUG_VECT;
    emit _vehicle->mavlinkMessageReceived(answer);
    QCOMPARE(advanceSpy.count(), 1);

    QVERIFY(scheduler.advanceBy(timeout));
    QCOMPARE(encoderCallCount, 2);
    QCOMPARE(timedOutSpy.count(), 0);
}

void VehicleMessageWaitStateTest::_testRequestThatCannotBeSentFails()
{
    // An offline vehicle has no link to send on
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    QGCStateMachine machine(QStringLiteral("TestMachine"), &vehicle);

    int encoderCallCount = 0;
    auto* const state = new WaitForMavlinkMessageState(&machine, MAVLINK_MSG_ID_DEBUG_VECT, 0);
    state->setRequest([&encoderCallCount](quint32, uint8_t, mavlink_message_t*) { encoderCallCount++; }, 1);
    finishOn(machine, state, &QGCState::error);

    QSignalSpy errorSpy(state, &QGCState::error);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No active link available")));
    machine.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(encoderCallCount, 0);
}

UT_REGISTER_TEST(VehicleMessageWaitStateTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Utilities)
