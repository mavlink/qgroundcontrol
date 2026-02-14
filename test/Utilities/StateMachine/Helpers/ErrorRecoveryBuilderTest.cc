#include "ErrorRecoveryBuilderTest.h"

#include "ErrorRecoveryBuilder.h"
#include "QGCStateMachine.h"

#include <QtTest/QSignalSpy>

namespace {
bool _spyTriggered(QSignalSpy& spy, int timeoutMsecs)
{
    return (spy.count() > 0) || spy.wait(timeoutMsecs);
}
}

void ErrorRecoveryBuilderTest::_testPrimarySuccessAdvances()
{
    QGCStateMachine machine(QStringLiteral("ErrorRecoveryPrimarySuccess"), nullptr);
    int primaryCalls = 0;

    auto* state = static_cast<ErrorRecoveryState*>(ErrorRecoveryBuilder(&machine, QStringLiteral("Recover"))
                                                       .withAction([&primaryCalls]() {
                                                           primaryCalls++;
                                                           return true;
                                                       })
                                                       .build());
    auto* finalState = machine.addFinalState();
    state->addTransition(state, &QGCState::advance, finalState);
    state->addTransition(state, &QGCState::error, finalState);
    machine.setInitialState(state);

    QSignalSpy succeededSpy(state, &ErrorRecoveryState::succeeded);
    QSignalSpy exhaustedSpy(state, &ErrorRecoveryState::exhausted);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(primaryCalls, 1);
    QCOMPARE(succeededSpy.count(), 1);
    QCOMPARE(exhaustedSpy.count(), 0);
    QCOMPARE(state->successPhase(), QStringLiteral("primary"));
}

void ErrorRecoveryBuilderTest::_testFallbackAfterRetriesSucceeds()
{
    QGCStateMachine machine(QStringLiteral("ErrorRecoveryFallback"), nullptr);
    int primaryCalls = 0;
    int fallbackCalls = 0;

    auto* state = static_cast<ErrorRecoveryState*>(ErrorRecoveryBuilder(&machine, QStringLiteral("Recover"))
                                                       .withAction([&primaryCalls]() {
                                                           primaryCalls++;
                                                           return false;
                                                       })
                                                       .retry(1, 1)
                                                       .withFallback([&fallbackCalls]() {
                                                           fallbackCalls++;
                                                           return true;
                                                       })
                                                       .build());
    auto* finalState = machine.addFinalState();
    state->addTransition(state, &QGCState::advance, finalState);
    state->addTransition(state, &QGCState::error, finalState);
    machine.setInitialState(state);

    QSignalSpy retryingSpy(state, &ErrorRecoveryState::retrying);
    QSignalSpy tryingFallbackSpy(state, &ErrorRecoveryState::tryingFallback);
    QSignalSpy succeededSpy(state, &ErrorRecoveryState::succeeded);
    QSignalSpy exhaustedSpy(state, &ErrorRecoveryState::exhausted);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(_spyTriggered(finishedSpy, 1000));
    QCOMPARE(primaryCalls, 2);
    QCOMPARE(fallbackCalls, 1);
    QCOMPARE(retryingSpy.count(), 1);
    QCOMPARE(tryingFallbackSpy.count(), 1);
    QCOMPARE(succeededSpy.count(), 1);
    QCOMPARE(exhaustedSpy.count(), 0);
    QCOMPARE(state->successPhase(), QStringLiteral("fallback"));
}

void ErrorRecoveryBuilderTest::_testFallbackFailureEndsInExhaustedError()
{
    QGCStateMachine machine(QStringLiteral("ErrorRecoveryFallbackFailure"), nullptr);
    int primaryCalls = 0;
    int fallbackCalls = 0;

    auto* state = static_cast<ErrorRecoveryState*>(ErrorRecoveryBuilder(&machine, QStringLiteral("Recover"))
                                                       .withAction([&primaryCalls]() {
                                                           primaryCalls++;
                                                           return false;
                                                       })
                                                       .retry(1, 1)
                                                       .withFallback([&fallbackCalls]() {
                                                           fallbackCalls++;
                                                           return false;
                                                       })
                                                       .build());
    auto* finalState = machine.addFinalState();
    state->addTransition(state, &QGCState::error, finalState);
    machine.setInitialState(state);

    QSignalSpy tryingFallbackSpy(state, &ErrorRecoveryState::tryingFallback);
    QSignalSpy exhaustedSpy(state, &ErrorRecoveryState::exhausted);
    QSignalSpy errorSpy(state, &QGCState::error);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(_spyTriggered(finishedSpy, 1000));
    QCOMPARE(primaryCalls, 2);
    QCOMPARE(fallbackCalls, 1);
    QCOMPARE(tryingFallbackSpy.count(), 1);
    QCOMPARE(exhaustedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(state->successPhase().isEmpty());
}

void ErrorRecoveryBuilderTest::_testExhaustedEmitAdvanceRunsRollback()
{
    QGCStateMachine machine(QStringLiteral("ErrorRecoveryExhaustedAdvance"), nullptr);
    int rollbackCalls = 0;

    auto* state = static_cast<ErrorRecoveryState*>(ErrorRecoveryBuilder(&machine, QStringLiteral("Recover"))
                                                       .withAction([]() {
                                                           return false;
                                                       })
                                                       .withRollback([&rollbackCalls]() {
                                                           rollbackCalls++;
                                                       })
                                                       .onExhausted(ErrorRecoveryBuilder::EmitAdvance)
                                                       .build());
    auto* finalState = machine.addFinalState();
    state->addTransition(state, &QGCState::advance, finalState);
    machine.setInitialState(state);

    QSignalSpy rollingBackSpy(state, &ErrorRecoveryState::rollingBack);
    QSignalSpy exhaustedSpy(state, &ErrorRecoveryState::exhausted);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(rollbackCalls, 1);
    QCOMPARE(rollingBackSpy.count(), 1);
    QCOMPARE(exhaustedSpy.count(), 1);
}

void ErrorRecoveryBuilderTest::_testExhaustedBehaviorMatrix()
{
    struct Case {
        ErrorRecoveryBuilder::ExhaustedBehavior behavior;
        bool expectAdvance;
    };

    const QList<Case> cases = {
        {ErrorRecoveryBuilder::EmitError, false},
        {ErrorRecoveryBuilder::EmitAdvance, true},
        {ErrorRecoveryBuilder::LogAndError, false},
        {ErrorRecoveryBuilder::LogAndAdvance, true},
    };

    for (const Case& tc : cases) {
        QGCStateMachine machine(QStringLiteral("ErrorRecoveryBehavior"), nullptr);

        auto* state = static_cast<ErrorRecoveryState*>(ErrorRecoveryBuilder(&machine, QStringLiteral("Recover"))
                                                           .withAction([]() { return false; })
                                                           .onExhausted(tc.behavior)
                                                           .build());
        auto* finalState = machine.addFinalState();
        state->addTransition(state, &QGCState::error, finalState);
        state->addTransition(state, &QGCState::advance, finalState);
        machine.setInitialState(state);

        QSignalSpy advanceSpy(state, &QGCState::advance);
        QSignalSpy errorSpy(state, &QGCState::error);
        QSignalSpy exhaustedSpy(state, &ErrorRecoveryState::exhausted);
        QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

        machine.start();

        QVERIFY(_spyTriggered(finishedSpy, 500));
        QCOMPARE(exhaustedSpy.count(), 1);
        QCOMPARE(advanceSpy.count() > 0, tc.expectAdvance);
        QCOMPARE(errorSpy.count() > 0, !tc.expectAdvance);
    }
}

void ErrorRecoveryBuilderTest::_testTimeoutExhaustsWithoutExtraRetries()
{
    QGCStateMachine machine(QStringLiteral("ErrorRecoveryTimeout"), nullptr);
    int primaryCalls = 0;

    auto* state = static_cast<ErrorRecoveryState*>(ErrorRecoveryBuilder(&machine, QStringLiteral("Recover"))
                                                       .withAction([&primaryCalls]() {
                                                           primaryCalls++;
                                                           return false;
                                                       })
                                                       .retry(5, 100)
                                                       .withTimeout(20)
                                                       .onExhausted(ErrorRecoveryBuilder::EmitAdvance)
                                                       .build());
    auto* finalState = machine.addFinalState();
    state->addTransition(state, &QGCState::advance, finalState);
    machine.setInitialState(state);

    QSignalSpy exhaustedSpy(state, &ErrorRecoveryState::exhausted);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(primaryCalls, 1);
    QCOMPARE(exhaustedSpy.count(), 1);
}

UT_REGISTER_TEST(ErrorRecoveryBuilderTest, TestLabel::Unit, TestLabel::Utilities)
