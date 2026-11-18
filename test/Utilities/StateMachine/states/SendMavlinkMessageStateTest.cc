#include "SendMavlinkMessageStateTest.h"

#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MockLink.h"
#include "SendMavlinkMessageState.h"
#include "StateTestCommon.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

void SendMavlinkMessageStateTest::_testSendsEncodedMessage()
{
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);
    const SharedLinkInterfacePtr link = _vehicle->vehicleLinkManager()->primaryLink().lock();
    QVERIFY(link);

    constexpr uint8_t debugIndex = 7;
    int encoderCallCount = 0;
    quint32 encoderSystemId = 0;
    int encoderChannel = -1;
    const auto encoder = [&](quint32 systemId, uint8_t channel, mavlink_message_t* message) {
        encoderCallCount++;
        encoderSystemId = systemId;
        encoderChannel = channel;
        (void) mavlink_msg_debug_pack_chan(systemId, MAVLinkProtocol::getComponentId(), channel, message, 0, debugIndex,
                                           0.0f);
    };

    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    auto* const state = new SendMavlinkMessageState(&machine, encoder, 0);
    state->addTransition(state, &QGCState::advance, new QFinalState(&machine));
    machine.setInitialState(state);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    _mockLink->clearReceivedMavlinkMessageCounts();
    machine.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());

    QCOMPARE(encoderCallCount, 1);
    QCOMPARE(encoderSystemId, MAVLinkProtocol::instance()->getSystemId());
    QCOMPARE(encoderChannel, static_cast<int>(link->mavlinkChannel()));

    QVERIFY_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_DEBUG) == 1, TestTimeout::mediumMs());
    mavlink_message_t received{};
    QVERIFY(_mockLink->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_DEBUG, received));
    QCOMPARE(mavlink_msg_debug_get_ind(&received), debugIndex);
}

void SendMavlinkMessageStateTest::_testFailsOnceRetriesAreUsedUp()
{
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);

    int encoderCallCount = 0;
    const auto encoder = [&encoderCallCount](quint32 systemId, uint8_t channel, mavlink_message_t* message) {
        encoderCallCount++;
        (void) mavlink_msg_debug_pack_chan(systemId, MAVLinkProtocol::getComponentId(), channel, message, 0, 0, 0.0f);
    };

    // Sends again each time it has sent, as a caller does that has not seen the reply it waits for
    constexpr int retryCount = 2;
    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    auto* const sendState = new SendMavlinkMessageState(&machine, encoder, retryCount);
    auto* const sendAgainState = new FunctionState(QStringLiteral("SendAgain"), &machine, []() {});
    sendState->addTransition(sendState, &QGCState::advance, sendAgainState);
    sendAgainState->addTransition(sendAgainState, &QGCState::advance, sendState);
    sendState->addTransition(sendState, &QGCState::error, new QFinalState(&machine));
    machine.setInitialState(sendState);

    QSignalSpy errorSpy(sendState, &QGCState::error);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    _mockLink->clearReceivedMavlinkMessageCounts();
    machine.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(encoderCallCount, retryCount + 1);
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_DEBUG) == retryCount + 1,
                      TestTimeout::mediumMs());
}

UT_REGISTER_TEST(SendMavlinkMessageStateTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Utilities)
