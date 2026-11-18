#include "RetryableRequestMessageStateTest.h"

#include <memory>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "MAVLinkLib.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "QGCStateMachine.h"
#include "RequestMessageCoordinator.h"
#include "StateTestCommon.h"

void RetryableRequestMessageStateTest::_testSuccessFirstAttempt()
{
    _connectMockLinkNoInitialConnectSequence();

    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle != nullptr);

    QGCStateMachine machine(QStringLiteral("TestMachine"), vehicle);
    bool messageReceived = false;

    _mockLink->setRequestMessageFailureMode(MockLink::FailRequestMessageNone);

    auto* requestState = new RetryableRequestMessageState(
        QStringLiteral("RequestDebug"),
        &machine,
        MAVLINK_MSG_ID_DEBUG,
        [&messageReceived](Vehicle*, const mavlink_message_t& msg) {
            QCOMPARE(msg.msgid, static_cast<uint32_t>(MAVLINK_MSG_ID_DEBUG));
            messageReceived = true;
        },
        2  // max retries
    );
    finishOn(machine, requestState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(TestTimeout::mediumMs()));
    QVERIFY(messageReceived);
    QCOMPARE(requestState->retryCount(), 0);  // No retries needed
    QCOMPARE(requestState->lastFailureCode(), Vehicle::RequestMessageNoFailure);

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testRetryOnFailure()
{
    // Every attempt ends either with the vehicle reporting the message as not received or with the state's timeout.
    ignoreLogMessage("Utilities.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("^Timeout \"TestMachine:RequestDebug\""));
    ignoreLogMessage("Utilities.StateMachine.RetryableRequestMessageState", QtWarningMsg,
                     QRegularExpression("Max retries exhausted"));
    _connectMockLinkNoInitialConnectSequence();

    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle != nullptr);

    QGCStateMachine machine(QStringLiteral("TestMachine"), vehicle);
    bool messageReceived = false;

    // First attempt will fail (message not sent), then succeed on retry
    // MockLink doesn't have a "fail once then succeed" mode, so we test the timeout retry path
    _mockLink->setRequestMessageFailureMode(MockLink::FailRequestMessageCommandAcceptedMsgNotSent);

    auto* requestState = new RetryableRequestMessageState(
        QStringLiteral("RequestDebug"), &machine, MAVLINK_MSG_ID_DEBUG,
        [&messageReceived](Vehicle*, const mavlink_message_t&) { messageReceived = true; },
        2,    // max retries
        MAV_COMP_ID_AUTOPILOT1,
        1000  // 1 second timeout per attempt
    );
    finishOn(machine, requestState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    // Should finish after retries are exhausted (failure mode always fails)
    QVERIFY(finishedSpy.wait(TestTimeout::longMs()));

    // With FailRequestMessageCommandAcceptedMsgNotSent, the result handler is called each time
    // but with failure code, so messageReceived stays false
    QVERIFY(!messageReceived);
    QCOMPARE(requestState->lastFailureCode(), Vehicle::RequestMessageFailureMessageNotReceived);

    _disconnectMockLink();
}

bool RetryableRequestMessageStateTest::_waitForNoAutopilotRequests()
{
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    return vehicle && UnitTest::waitForCondition(
                          [vehicle]() {
                              return !vehicle->_reqMsgCoord->hasPendingRequests(MAV_COMP_ID_AUTOPILOT1) &&
                                     !vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE);
                          },
                          TestTimeout::longMs(), QStringLiteral("no requests to the autopilot outstanding"));
}

void RetryableRequestMessageStateTest::_sendWithheldDebugReply()
{
    _mockLink->sendUnexpectedCommandAck(MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);
    mavlink_message_t debugMessage{};
    (void) mavlink_msg_debug_pack_chan(static_cast<uint8_t>(_mockLink->vehicleId()), MAV_COMP_ID_AUTOPILOT1,
                                       _mockLink->outgoingMavlinkChannel(), &debugMessage, 0, 0, 0);
    _mockLink->respondWithMavlinkMessage(debugMessage);
}

void RetryableRequestMessageStateTest::_testRetryAfterTimeout_data()
{
    QTest::addColumn<bool>("timedOutRequestAnswered");
    QTest::addColumn<bool>("afterSecondTimeoutInterval");

    QTest::newRow("timed out request fails") << false << false;
    QTest::newRow("timed out request answered late") << true << false;
    QTest::newRow("timed out request fails after a second timeout interval") << false << true;
}

void RetryableRequestMessageStateTest::_testRetryAfterTimeout()
{
    QFETCH(bool, timedOutRequestAnswered);
    QFETCH(bool, afterSecondTimeoutInterval);

    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);
    QVERIFY(_waitForNoAutopilotRequests());

    // The vehicle does not answer and the command queue does not give up while the test runs, so only the
    // state's own timeout ends the first attempt.
    TestFixtures::MavCommandAckTimeoutFixture ackTimeout(TestTimeout::longMs());
    _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_DEBUG);
    _mockLink->clearReceivedMavCommandCounts();

    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    int handlerCallCount = 0;
    // One retry, as the initial connect sequence uses
    const int maxRetries = 1;
    auto* requestState = new RetryableRequestMessageState(
        QStringLiteral("RequestDebug"), &machine, MAVLINK_MSG_ID_DEBUG,
        [&handlerCallCount](Vehicle*, const mavlink_message_t&) { handlerCallCount++; }, maxRetries,
        MAV_COMP_ID_AUTOPILOT1, TestTimeout::shortMs());
    finishOn(machine, requestState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    expectLogMessage("Utilities.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("^Timeout \"TestMachine:RequestDebug\""));
    machine.start();

    QVERIFY_TRUE_WAIT(requestState->retryCount() == 1, TestTimeout::shortMs() + TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG), 1);

    if (afterSecondTimeoutInterval) {
        // Vehicle still tracks the first request, so the retry cannot be sent yet and must not be given up
        QVERIFY_NO_SIGNAL_WAIT(finishedSpy, TestTimeout::shortMs() * 5 / 4);
        QCOMPARE(requestState->retryCount(), 1);
        QCOMPARE(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG), 1);
    }

    _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_DEBUG, false);
    if (timedOutRequestAnswered) {
        _sendWithheldDebugReply();
    } else {
        _mockLink->sendUnexpectedCommandAck(MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_FAILED);
    }

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(handlerCallCount, 1);
    QCOMPARE(requestState->lastFailureCode(), Vehicle::RequestMessageNoFailure);
    QCOMPARE(requestState->retryCount(), 1);
    QCOMPARE(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG),
             timedOutRequestAnswered ? 1 : 2);

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testRefusedRequestEndsAtItsTimeout()
{
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);
    QVERIFY(_waitForNoAutopilotRequests());

    // Another request for the message is outstanding until the test sends the reply, so the vehicle refuses the
    // request of the state
    TestFixtures::MavCommandAckTimeoutFixture ackTimeout(TestTimeout::longMs());
    _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_DEBUG);
    _mockLink->clearReceivedMavCommandCounts();
    int otherRequestResults = 0;
    _vehicle->requestMessage([](void* resultHandlerData, MAV_RESULT, Vehicle::RequestMessageResultHandlerFailureCode_t,
                                const mavlink_message_t&) { ++*static_cast<int*>(resultHandlerData); },
                             &otherRequestResults, MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG);
    QVERIFY_TRUE_WAIT(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG) == 1,
                      TestTimeout::shortMs());

    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    int handlerCallCount = 0;
    const int maxRetries = 1;
    auto* requestState = new RetryableRequestMessageState(
        QStringLiteral("RequestDebug"), &machine, MAVLINK_MSG_ID_DEBUG,
        [&handlerCallCount](Vehicle*, const mavlink_message_t&) { handlerCallCount++; }, maxRetries,
        MAV_COMP_ID_AUTOPILOT1, TestTimeout::shortMs());
    finishOn(machine, requestState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    expectLogMessage("Vehicle.RequestMessageCoordinator", QtWarningMsg, QRegularExpression("failing exact duplicate"));
    QVERIFY(startAndWaitForEntered(&machine, requestState));
    verifyExpectedLogMessage();
    QCOMPARE(requestState->lastFailureCode(), Vehicle::RequestMessageFailureDuplicate);

    // The other request is answered while the refused attempt waits for its timeout
    _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_DEBUG, false);
    _sendWithheldDebugReply();
    QVERIFY_TRUE_WAIT(otherRequestResults == 1, TestTimeout::shortMs());
    QCOMPARE(requestState->retryCount(), 0);
    QCOMPARE(finishedSpy.count(), 0);

    // The retry is sent at the timeout, when the vehicle takes it
    expectLogMessage("Utilities.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("^Timeout \"TestMachine:RequestDebug\""));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(handlerCallCount, 1);
    QCOMPARE(requestState->retryCount(), 1);
    QCOMPARE(requestState->lastFailureCode(), Vehicle::RequestMessageNoFailure);
    QCOMPARE(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG), 2);
    QCOMPARE(otherRequestResults, 1);

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testMaxRetriesExhausted()
{
    TestFixtures::MavCommandAckTimeoutFixture shortAckTimeout;
    // The state times out before the command queue gives up on the first request, then both attempts go unanswered.
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));
    ignoreLogMessage("Utilities.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("^Timeout \"TestMachine:RequestDebug\""));
    ignoreLogMessage("Utilities.StateMachine.RetryableRequestMessageState", QtWarningMsg,
                     QRegularExpression("Max retries exhausted"));
    _connectMockLinkNoInitialConnectSequence();

    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle != nullptr);

    QGCStateMachine machine(QStringLiteral("TestMachine"), vehicle);
    bool retriesExhaustedEmitted = false;

    _mockLink->setRequestMessageFailureMode(MockLink::FailRequestMessageCommandNoResponse);

    auto* requestState = new RetryableRequestMessageState(
        QStringLiteral("RequestDebug"),
        &machine,
        MAVLINK_MSG_ID_DEBUG,
        nullptr,
        1,                      // max 1 retry (so 2 attempts total)
        MAV_COMP_ID_AUTOPILOT1,
        500                     // Short timeout
    );

    connect(requestState, &RetryableRequestMessageState::retriesExhausted, this, [&retriesExhaustedEmitted]() {
        retriesExhaustedEmitted = true;
    });

    // Default behavior: advance even after max retries (graceful degradation)
    finishOn(machine, requestState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(TestTimeout::longMs()));
    QVERIFY(retriesExhaustedEmitted);
    QCOMPARE(requestState->retryCount(), 1);  // One retry was performed

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testFailOnMaxRetries()
{
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);
    QVERIFY(_waitForNoAutopilotRequests());

    // The vehicle does not answer and the command queue does not give up while the test runs, so the failure
    // can only come from the state's own timeout.
    TestFixtures::MavCommandAckTimeoutFixture ackTimeout(TestTimeout::longMs());
    _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_DEBUG);

    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    int failureHandlerCallCount = 0;

    auto* requestState = new RetryableRequestMessageState(
        QStringLiteral("RequestDebug"),
        &machine,
        MAVLINK_MSG_ID_DEBUG,
        nullptr,
        0,                      // No retries
        MAV_COMP_ID_AUTOPILOT1,
        500
    );
    requestState->setFailOnMaxRetries(true);  // Emit error() instead of advance()
    requestState->setFailureHandler([&failureHandlerCallCount](auto, auto) { failureHandlerCallCount++; });
    // Without transitions the state stays active after it has reported the failure
    machine.setInitialState(requestState);

    QSignalSpy errorSpy(requestState, &QGCState::error);
    QSignalSpy advanceSpy(requestState, &QGCState::advance);
    QSignalSpy retriesExhaustedSpy(requestState, &RetryableRequestMessageState::retriesExhausted);
    QSignalSpy messageReceivedSpy(requestState, &RetryableRequestMessageState::messageReceived);
    expectLogMessage("Utilities.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("^Timeout \"TestMachine:RequestDebug\""));
    expectLogMessage("Utilities.StateMachine.RetryableRequestMessageState", QtWarningMsg,
                     QRegularExpression("Max retries exhausted"));
    machine.start();

    QVERIFY_SIGNAL_COUNT_WAIT(errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(advanceSpy.count(), 0);

    // A reply to the request the state has given up on changes nothing
    _sendWithheldDebugReply();
    QVERIFY(_waitForNoAutopilotRequests());
    QCOMPARE(messageReceivedSpy.count(), 0);
    QCOMPARE(failureHandlerCallCount, 1);
    QCOMPARE(retriesExhaustedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testStateDestroyedWhileRequestOutstanding()
{
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);
    QVERIFY(_waitForNoAutopilotRequests());

    // The request stays outstanding until the test sends the reply
    TestFixtures::MavCommandAckTimeoutFixture ackTimeout(TestTimeout::longMs());
    _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_DEBUG);
    _mockLink->clearReceivedMavCommandCounts();

    auto machine = std::make_unique<QGCStateMachine>(QStringLiteral("TestMachine"), _vehicle);
    int handlerCallCount = 0;
    auto* requestState = new RetryableRequestMessageState(
        QStringLiteral("RequestDebug"), machine.get(), MAVLINK_MSG_ID_DEBUG,
        [&handlerCallCount](Vehicle*, const mavlink_message_t&) { handlerCallCount++; });
    finishOn(*machine, requestState, &QGCState::advance);
    machine->start();
    QVERIFY_TRUE_WAIT(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG) == 1,
                      TestTimeout::shortMs());

    // The vehicle holds a pointer to the state for the reply, which must not be used once the state is gone
    machine.reset();
    _sendWithheldDebugReply();
    QVERIFY(_waitForNoAutopilotRequests());
    QCOMPARE(handlerCallCount, 0);

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testVehicleDestroyedWhileMachineAlive_data()
{
    QTest::addColumn<bool>("requestOutstanding");

    QTest::newRow("before the machine starts") << false;
    QTest::newRow("while the request is outstanding") << true;
}

void RetryableRequestMessageStateTest::_testVehicleDestroyedWhileMachineAlive()
{
    QFETCH(bool, requestOutstanding);

    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);

    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    auto* requestState =
        new RetryableRequestMessageState(QStringLiteral("RequestDebug"), &machine, MAVLINK_MSG_ID_DEBUG);
    finishOn(machine, requestState, &QGCState::error);
    QCOMPARE(machine.vehicle(), _vehicle);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    // Only the loss of the vehicle ends the request
    TestFixtures::MavCommandAckTimeoutFixture ackTimeout(TestTimeout::longMs());
    if (requestOutstanding) {
        QVERIFY(_waitForNoAutopilotRequests());
        _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_DEBUG);
        _mockLink->clearReceivedMavCommandCounts();
        machine.start();
        QVERIFY_TRUE_WAIT(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG) == 1,
                          TestTimeout::shortMs());
        expectLogMessage("Utilities.StateMachine.RetryableRequestMessageState", QtWarningMsg,
                         QRegularExpression("Vehicle destroyed while requesting message"));
    }

    const QPointer<Vehicle> vehicleGuard(_vehicle);
    _disconnectMockLink();
    QVERIFY_TRUE_WAIT(!vehicleGuard, TestTimeout::mediumMs());
    QVERIFY(!machine.vehicle());

    if (!requestOutstanding) {
        expectLogMessage("Utilities.StateMachine.RetryableRequestMessageState", QtWarningMsg,
                         QRegularExpression("No vehicle available"));
        machine.start();
    }
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(RetryableRequestMessageStateTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Utilities)
