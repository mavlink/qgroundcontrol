#include "AdditionalStatesCoverageTest.h"
#include "StateTestCommon.h"

#include "CircuitBreakerState.h"
#include "EventQueuedState.h"
#include "FallbackChainState.h"
#include "LoopState.h"
#include "RetryState.h"
#include "RollbackState.h"
#include "SequenceState.h"

void AdditionalStatesCoverageTest::_testCircuitBreakerTripsAndFailFast()
{
    QStateMachine machine;
    int actionCallCount = 0;

    auto* breaker = new CircuitBreakerState(
        QStringLiteral("CircuitBreaker"), &machine,
        [&actionCallCount]() {
            actionCallCount++;
            return false;
        },
        1,      // trip immediately on first failure
        1000    // keep open long enough to verify fail-fast on second run
    );
    auto* finalState = new QFinalState(&machine);

    breaker->addTransition(breaker, &QGCState::error, finalState);
    machine.setInitialState(breaker);

    QSignalSpy firstFinishedSpy(&machine, &QStateMachine::finished);
    QSignalSpy trippedSpy(breaker, &CircuitBreakerState::tripped);

    machine.start();

    QVERIFY(firstFinishedSpy.wait(500));
    QCOMPARE(actionCallCount, 1);
    QCOMPARE(trippedSpy.count(), 1);
    QVERIFY(breaker->isTripped());

    QSignalSpy secondFinishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(secondFinishedSpy.wait(500));
    QCOMPARE(actionCallCount, 1);  // still open, should fail fast without action call
}

void AdditionalStatesCoverageTest::_testEventQueuedStateMatchesExpectedEvent()
{
    QStateMachine machine;

    auto* waitState = new EventQueuedState(QStringLiteral("WaitEvent"), &machine, QStringLiteral("ready"), 0);
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::completed, finalState);
    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy eventReceivedSpy(waitState, &EventQueuedState::eventReceived);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    QVERIFY(QMetaObject::invokeMethod(waitState, "_onMachineEvent", Q_ARG(QString, QStringLiteral("ignore"))));
    QTest::qWait(20);
    QCOMPARE(eventReceivedSpy.count(), 0);

    QVERIFY(QMetaObject::invokeMethod(waitState, "_onMachineEvent", Q_ARG(QString, QStringLiteral("ready"))));

    QVERIFY((finishedSpy.count() > 0) || finishedSpy.wait(500));
    QCOMPARE(eventReceivedSpy.count(), 1);
    QCOMPARE(waitState->receivedEvent(), QStringLiteral("ready"));
}

void AdditionalStatesCoverageTest::_testFallbackChainUsesFallbackStrategy()
{
    QStateMachine machine;
    QStringList attemptedStrategies;

    auto* fallbackState = new FallbackChainState(QStringLiteral("Fallback"), &machine);
    auto* finalState = new QFinalState(&machine);

    fallbackState->addStrategy(QStringLiteral("first"), [&attemptedStrategies]() {
        attemptedStrategies.append(QStringLiteral("first"));
        return false;
    });
    fallbackState->addStrategy(QStringLiteral("second"), [&attemptedStrategies]() {
        attemptedStrategies.append(QStringLiteral("second"));
        return true;
    });

    fallbackState->addTransition(fallbackState, &QGCState::advance, finalState);
    fallbackState->addTransition(fallbackState, &QGCState::error, finalState);
    machine.setInitialState(fallbackState);

    QSignalSpy strategyFailedSpy(fallbackState, &FallbackChainState::strategyFailed);
    QSignalSpy strategySucceededSpy(fallbackState, &FallbackChainState::strategySucceeded);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(attemptedStrategies, QStringList({QStringLiteral("first"), QStringLiteral("second")}));
    QCOMPARE(strategyFailedSpy.count(), 1);
    QCOMPARE(strategySucceededSpy.count(), 1);
    QCOMPARE(fallbackState->successfulStrategy(), QStringLiteral("second"));
}

void AdditionalStatesCoverageTest::_testLoopStateProcessesFilteredItems()
{
    QStateMachine machine;
    QList<int> processedItems;

    auto* loopState = new LoopState<int>(
        QStringLiteral("Loop"), &machine,
        QList<int>{1, 2, 3, 4},
        [&processedItems](const int& value, int) {
            processedItems.append(value);
        }
    );
    auto* finalState = new QFinalState(&machine);

    loopState->setFilter([](const int& value, int) {
        return (value % 2) == 0;
    });

    loopState->addTransition(loopState, &QGCState::advance, finalState);
    machine.setInitialState(loopState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(processedItems, QList<int>({2, 4}));
    QCOMPARE(loopState->processedCount(), 2);
    QCOMPARE(loopState->totalItems(), 4);
}

void AdditionalStatesCoverageTest::_testRetryThenFailEmitsExhausted()
{
    QStateMachine machine;
    int attemptCount = 0;

    auto* retryState = new RetryState(
        QStringLiteral("RetryThenFail"), &machine,
        [&attemptCount]() {
            attemptCount++;
            return false;
        },
        2,   // total attempts should be 3
        10,
        RetryState::EmitError
    );
    auto* finalState = new QFinalState(&machine);

    retryState->addTransition(retryState, &QGCState::error, finalState);
    machine.setInitialState(retryState);

    QSignalSpy retryingSpy(retryState, &RetryState::retrying);
    QSignalSpy exhaustedSpy(retryState, &RetryState::exhausted);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(finishedSpy.wait(1000));
    QCOMPARE(attemptCount, 3);
    QCOMPARE(retryingSpy.count(), 2);
    QCOMPARE(exhaustedSpy.count(), 1);
    QCOMPARE(retryState->currentAttempt(), 3);
}

void AdditionalStatesCoverageTest::_testRetryThenSkipAdvancesAfterSkip()
{
    QStateMachine machine;
    int attemptCount = 0;

    auto* retrySkipState = new RetryState(
        QStringLiteral("RetryThenSkip"), &machine,
        [&attemptCount]() {
            attemptCount++;
            return false;
        },
        1,   // total attempts should be 2
        10,
        RetryState::EmitAdvance
    );
    auto* finalState = new QFinalState(&machine);

    retrySkipState->addTransition(retrySkipState, &QGCState::advance, finalState);
    machine.setInitialState(retrySkipState);

    QSignalSpy skippedSpy(retrySkipState, &RetryState::skipped);
    QSignalSpy succeededSpy(retrySkipState, &RetryState::succeeded);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(finishedSpy.wait(1000));
    QCOMPARE(attemptCount, 2);
    QCOMPARE(skippedSpy.count(), 1);
    QCOMPARE(succeededSpy.count(), 0);
    QVERIFY(retrySkipState->wasSkipped());
}

void AdditionalStatesCoverageTest::_testRollbackStateExecutesRollbackOnFailure()
{
    QStateMachine machine;
    bool rollbackCalled = false;

    auto* rollbackState = new RollbackState(
        QStringLiteral("Rollback"), &machine,
        []() {
            return false;
        },
        [&rollbackCalled]() {
            rollbackCalled = true;
        }
    );
    auto* finalState = new QFinalState(&machine);

    rollbackState->addTransition(rollbackState, &QGCState::error, finalState);
    machine.setInitialState(rollbackState);

    QSignalSpy forwardFailedSpy(rollbackState, &RollbackState::forwardFailed);
    QSignalSpy rollingBackSpy(rollbackState, &RollbackState::rollingBack);
    QSignalSpy rollbackCompleteSpy(rollbackState, &RollbackState::rollbackComplete);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(rollbackCalled);
    QVERIFY(rollbackState->wasRolledBack());
    QVERIFY(rollbackState->rollbackSucceeded());
    QCOMPARE(forwardFailedSpy.count(), 1);
    QCOMPARE(rollingBackSpy.count(), 1);
    QCOMPARE(rollbackCompleteSpy.count(), 1);
}

void AdditionalStatesCoverageTest::_testSequenceStateStopsOnFailedStep()
{
    QStateMachine machine;
    QStringList executedSteps;

    auto* sequenceState = new SequenceState(QStringLiteral("Sequence"), &machine);
    auto* finalState = new QFinalState(&machine);

    sequenceState->addStep(QStringLiteral("step1"), [&executedSteps]() {
        executedSteps.append(QStringLiteral("step1"));
        return true;
    });
    sequenceState->addStep(QStringLiteral("step2"), [&executedSteps]() {
        executedSteps.append(QStringLiteral("step2"));
        return false;
    });
    sequenceState->addStep(QStringLiteral("step3"), [&executedSteps]() {
        executedSteps.append(QStringLiteral("step3"));
        return true;
    });

    sequenceState->addTransition(sequenceState, &QGCState::advance, finalState);
    sequenceState->addTransition(sequenceState, &QGCState::error, finalState);
    machine.setInitialState(sequenceState);

    QSignalSpy stepCompletedSpy(sequenceState, &SequenceState::stepCompleted);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(executedSteps, QStringList({QStringLiteral("step1"), QStringLiteral("step2")}));
    QCOMPARE(stepCompletedSpy.count(), 1);
    QCOMPARE(sequenceState->failedStepName(), QStringLiteral("step2"));
}

UT_REGISTER_TEST(AdditionalStatesCoverageTest, TestLabel::Unit, TestLabel::Utilities)
