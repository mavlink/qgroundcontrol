#include "SkippableAsyncStateTest.h"
#include "StateTestCommon.h"


void SkippableAsyncStateTest::_testSkippableAsyncStateExecute()
{
    QStateMachine machine;
    bool setupCalled = false;
    bool skipPredicateCalled = false;
    SkippableAsyncState* capturedState = nullptr;

    auto* skippableState = new SkippableAsyncState(
        QStringLiteral("TestSkippableAsync"),
        &machine,
        [&skipPredicateCalled]() {
            skipPredicateCalled = true;
            return false;  // Don't skip
        },
        [&setupCalled, &capturedState](SkippableAsyncState* state) {
            setupCalled = true;
            capturedState = state;
            QTimer::singleShot(50, state, [state]() {
                state->complete();
            });
        }
    );
    auto* finalState = new QFinalState(&machine);

    skippableState->addTransition(skippableState, &QGCState::advance, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::skipped, finalState);
    machine.setInitialState(skippableState);

    // Use MultiSignalSpy to verify advance() fired but skipped() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(skippableState));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(skipPredicateCalled);
    QVERIFY(setupCalled);
    QVERIFY(capturedState != nullptr);
    // Verify advance path taken, not skip path
    QVERIFY(stateSpy.emittedByMask(stateSpy.mask("advance")));
    QVERIFY(stateSpy.notEmittedByMask(stateSpy.mask("skipped")));
}

void SkippableAsyncStateTest::_testSkippableAsyncStateSkip()
{
    QStateMachine machine;
    bool setupCalled = false;
    bool skipPredicateCalled = false;
    bool skipHandled = false;

    auto* skippableState = new SkippableAsyncState(
        QStringLiteral("TestSkippableAsyncSkip"),
        &machine,
        [&skipPredicateCalled]() {
            skipPredicateCalled = true;
            return true;  // Skip
        },
        [&setupCalled](SkippableAsyncState* state) {
            Q_UNUSED(state);
            setupCalled = true;  // Should NOT be called
        }
    );
    auto* skipState = new FunctionState(QStringLiteral("SkipHandler"), &machine, [&skipHandled]() {
        skipHandled = true;
    });
    auto* finalState = new QFinalState(&machine);

    skippableState->addTransition(skippableState, &QGCState::advance, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::skipped, skipState);
    skipState->addTransition(skipState, &QGCState::advance, finalState);
    machine.setInitialState(skippableState);

    // Use MultiSignalSpy to verify skipped() fired but advance() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(skippableState));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(skipPredicateCalled);
    QVERIFY(!setupCalled);  // Setup should NOT have been called
    QVERIFY(skipHandled);
    // Verify skip path taken, not execute path
    QVERIFY(stateSpy.emittedByMask(stateSpy.mask("skipped")));
    QVERIFY(stateSpy.notEmittedByMask(stateSpy.mask("advance")));
}

void SkippableAsyncStateTest::_testSkippableAsyncStateTimeout()
{
    QStateMachine machine;
    bool timeoutReached = false;
    const int timeoutMs = 100;

    auto* skippableState = new SkippableAsyncState(
        QStringLiteral("TestSkippableAsyncTimeout"),
        &machine,
        []() { return false; },  // Don't skip
        [](SkippableAsyncState* state) {
            Q_UNUSED(state);
            // Don't call complete() - let it timeout
        },
        nullptr,
        timeoutMs
    );
    auto* timeoutState = new FunctionState(QStringLiteral("TimeoutHandler"), &machine, [&timeoutReached]() {
        timeoutReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    skippableState->addTransition(skippableState, &QGCState::advance, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::skipped, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::timeout, timeoutState);
    timeoutState->addTransition(timeoutState, &QGCState::advance, finalState);
    machine.setInitialState(skippableState);

    // Use MultiSignalSpy to verify timeout() fired but advance() and skipped() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(skippableState));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(timeoutReached);
    // Verify timeout path taken
    QVERIFY(stateSpy.emittedByMask(stateSpy.mask("timeout")));
    QVERIFY(stateSpy.notEmittedByMask(stateSpy.mask("advance")));
    QVERIFY(stateSpy.notEmittedByMask(stateSpy.mask("skipped")));
}

void SkippableAsyncStateTest::_testSkippableAsyncStateWithSkipAction()
{
    QStateMachine machine;
    bool skipActionCalled = false;
    bool setupCalled = false;

    auto* skippableState = new SkippableAsyncState(
        QStringLiteral("TestSkippableAsyncWithSkipAction"),
        &machine,
        []() { return true; },  // Skip
        [&setupCalled](SkippableAsyncState* state) {
            Q_UNUSED(state);
            setupCalled = true;  // Should NOT be called
        },
        [&skipActionCalled]() {
            skipActionCalled = true;  // Should be called
        }
    );
    auto* finalState = new QFinalState(&machine);

    skippableState->addTransition(skippableState, &QGCState::advance, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::skipped, finalState);
    machine.setInitialState(skippableState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(skipActionCalled);  // Skip action should have been called
    QVERIFY(!setupCalled);      // Setup should NOT have been called
}

UT_REGISTER_TEST(SkippableAsyncStateTest, TestLabel::Unit, TestLabel::Utilities)
