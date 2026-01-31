#include "RetryableRequestMessageStateTest.h"
#include "StateTestCommon.h"

#include "QGCStateMachine.h"
#include "MultiVehicleManager.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>

void RetryableRequestMessageStateTest::_testSuccessFirstAttempt()
{
    _connectMockLinkNoInitialConnectSequence();

    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle != nullptr);

    // Disable other components that send message requests
    vehicle->_deleteGimbalController();
    vehicle->_deleteCameraManager();

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
    auto* finalState = new QFinalState(&machine);

    requestState->addTransition(requestState, &QGCState::advance, finalState);
    machine.setInitialState(requestState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(5000));
    QVERIFY(messageReceived);
    QCOMPARE(requestState->retryCount(), 0);  // No retries needed
    QCOMPARE(requestState->lastFailureCode(), Vehicle::RequestMessageNoFailure);

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testRetryOnFailure()
{
    _connectMockLinkNoInitialConnectSequence();

    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle != nullptr);

    vehicle->_deleteGimbalController();
    vehicle->_deleteCameraManager();

    QGCStateMachine machine(QStringLiteral("TestMachine"), vehicle);
    bool messageReceived = false;
    int handlerCallCount = 0;

    // First attempt will fail (message not sent), then succeed on retry
    // MockLink doesn't have a "fail once then succeed" mode, so we test the timeout retry path
    _mockLink->setRequestMessageFailureMode(MockLink::FailRequestMessageCommandAcceptedMsgNotSent);

    auto* requestState = new RetryableRequestMessageState(
        QStringLiteral("RequestDebug"),
        &machine,
        MAVLINK_MSG_ID_DEBUG,
        [&messageReceived, &handlerCallCount](Vehicle*, const mavlink_message_t&) {
            handlerCallCount++;
            messageReceived = true;
        },
        2,                      // max retries
        MAV_COMP_ID_AUTOPILOT1,
        1000                    // 1 second timeout per attempt
    );
    auto* finalState = new QFinalState(&machine);

    requestState->addTransition(requestState, &QGCState::advance, finalState);
    machine.setInitialState(requestState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    // Should finish after retries are exhausted (failure mode always fails)
    QVERIFY(finishedSpy.wait(10000));

    // With FailRequestMessageCommandAcceptedMsgNotSent, the result handler is called each time
    // but with failure code, so messageReceived stays false
    QVERIFY(!messageReceived);
    QCOMPARE(requestState->lastFailureCode(), Vehicle::RequestMessageFailureMessageNotReceived);

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testMaxRetriesExhausted()
{
    _connectMockLinkNoInitialConnectSequence();

    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle != nullptr);

    vehicle->_deleteGimbalController();
    vehicle->_deleteCameraManager();

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
    auto* finalState = new QFinalState(&machine);

    connect(requestState, &RetryableRequestMessageState::retriesExhausted, this, [&retriesExhaustedEmitted]() {
        retriesExhaustedEmitted = true;
    });

    // Default behavior: advance even after max retries (graceful degradation)
    requestState->addTransition(requestState, &QGCState::advance, finalState);
    machine.setInitialState(requestState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(10000));
    QVERIFY(retriesExhaustedEmitted);
    QCOMPARE(requestState->retryCount(), 1);  // One retry was performed

    _disconnectMockLink();
}

void RetryableRequestMessageStateTest::_testFailOnMaxRetries()
{
    _connectMockLinkNoInitialConnectSequence();

    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle != nullptr);

    vehicle->_deleteGimbalController();
    vehicle->_deleteCameraManager();

    QGCStateMachine machine(QStringLiteral("TestMachine"), vehicle);
    bool errorStateReached = false;
    bool successStateReached = false;

    _mockLink->setRequestMessageFailureMode(MockLink::FailRequestMessageCommandNoResponse);

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

    auto* successState = new FunctionState(QStringLiteral("Success"), &machine, [&successStateReached]() {
        successStateReached = true;
    });
    auto* errorState = new FunctionState(QStringLiteral("Error"), &machine, [&errorStateReached]() {
        errorStateReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    requestState->addTransition(requestState, &QGCState::advance, successState);
    requestState->addTransition(requestState, &QGCState::error, errorState);
    successState->addTransition(successState, &QGCState::advance, finalState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);
    machine.setInitialState(requestState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(10000));
    QVERIFY(errorStateReached);
    QVERIFY(!successStateReached);

    _disconnectMockLink();
}
