#include "RetryTransitionTest.h"
#include "TransitionTestCommon.h"

void RetryTransitionTest::_testRetryActionCalled()
{
    QStateMachine machine;
    int retryCount = 0;
    bool targetReached = false;

    // Create a state that will timeout (we never call complete())
    auto* startState = new AsyncFunctionState(
        QStringLiteral("Start"),
        &machine,
        [](AsyncFunctionState*) {
            // Don't complete - let it timeout
        },
        50  // 50ms timeout
    );

    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&targetReached]() {
        targetReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    // RetryTransition with 1 retry
    auto* retryTransition = new RetryTransition(
        startState, &WaitStateBase::timeout,
        targetState,
        [&retryCount]() { retryCount++; },
        1  // max 1 retry
    );
    startState->addTransition(retryTransition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    // Should finish after retry + final transition
    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(retryCount, 1);  // Retry action called once
    QVERIFY(targetReached);   // Then transitioned to target
}

void RetryTransitionTest::_testTransitionAfterMaxRetries()
{
    QStateMachine machine;
    int retryCount = 0;
    bool targetReached = false;
    bool alternateReached = false;

    auto* startState = new AsyncFunctionState(
        QStringLiteral("Start"),
        &machine,
        [](AsyncFunctionState*) {},
        30  // 30ms timeout
    );

    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&targetReached]() {
        targetReached = true;
    });
    auto* alternateState = new FunctionState(QStringLiteral("Alternate"), &machine, [&alternateReached]() {
        alternateReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    // RetryTransition goes to targetState after retries exhausted
    auto* retryTransition = new RetryTransition(
        startState, &WaitStateBase::timeout,
        targetState,
        [&retryCount]() { retryCount++; },
        2  // max 2 retries
    );
    startState->addTransition(retryTransition);

    // This transition should NOT be taken (RetryTransition should win after retries)
    startState->addTransition(startState, &QGCState::advance, alternateState);

    targetState->addTransition(targetState, &QGCState::advance, finalState);
    alternateState->addTransition(alternateState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(retryCount, 2);      // Both retries used
    QVERIFY(targetReached);       // Transitioned to retry target
    QVERIFY(!alternateReached);   // Not to alternate
}

void RetryTransitionTest::_testRetryCountResets()
{
    // Test that retry count resets after transition is taken
    QStateMachine machine;
    int retryCount = 0;

    auto* startState = new AsyncFunctionState(
        QStringLiteral("Start"),
        &machine,
        [](AsyncFunctionState*) {},
        30
    );

    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, []() {});
    auto* finalState = new QFinalState(&machine);

    auto* retryTransition = new RetryTransition(
        startState, &WaitStateBase::timeout,
        targetState,
        [&retryCount]() { retryCount++; },
        1
    );
    startState->addTransition(retryTransition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));

    // Verify the transition's internal count was reset
    QCOMPARE(retryTransition->retryCount(), 0);
}

void RetryTransitionTest::_testMultipleRetries()
{
    QStateMachine machine;
    int retryCount = 0;
    bool targetReached = false;

    auto* startState = new AsyncFunctionState(
        QStringLiteral("Start"),
        &machine,
        [](AsyncFunctionState*) {},
        20  // Short timeout for faster test
    );

    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&targetReached]() {
        targetReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    // Test with 3 retries
    auto* retryTransition = new RetryTransition(
        startState, &WaitStateBase::timeout,
        targetState,
        [&retryCount]() { retryCount++; },
        3  // max 3 retries
    );
    startState->addTransition(retryTransition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(1000));  // Longer timeout for multiple retries
    QCOMPARE(retryCount, 3);  // All 3 retries used
    QVERIFY(targetReached);
}

void RetryTransitionTest::_testZeroRetries()
{
    QStateMachine machine;
    int retryCount = 0;
    bool targetReached = false;

    auto* startState = new AsyncFunctionState(
        QStringLiteral("Start"),
        &machine,
        [](AsyncFunctionState*) {},
        30
    );

    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&targetReached]() {
        targetReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    // Zero retries = immediate transition on first timeout
    auto* retryTransition = new RetryTransition(
        startState, &WaitStateBase::timeout,
        targetState,
        [&retryCount]() { retryCount++; },
        0  // max 0 retries
    );
    startState->addTransition(retryTransition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(retryCount, 0);  // No retries attempted
    QVERIFY(targetReached);   // Immediate transition
}
