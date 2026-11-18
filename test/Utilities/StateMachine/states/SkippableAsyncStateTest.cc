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
    auto* finalState = addFinalState(&machine);

    skippableState->addTransition(skippableState, &QGCState::advance, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::skipped, finalState);
    machine.setInitialState(skippableState);

    // Use MultiSignalSpy to verify advance() fired but skipped() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(skippableState));

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(skipPredicateCalled);
    QVERIFY(setupCalled);
    QVERIFY(capturedState != nullptr);
    QVERIFY_SIGNAL_EMITTED(stateSpy, "advance");
    QVERIFY_NO_SIGNAL(stateSpy, "skipped");
}

void SkippableAsyncStateTest::_testSkippableAsyncStateSkip_data()
{
    QTest::addColumn<bool>("withSkipAction");

    QTest::newRow("without skip action") << false;
    QTest::newRow("with skip action") << true;
}

void SkippableAsyncStateTest::_testSkippableAsyncStateSkip()
{
    QFETCH(bool, withSkipAction);

    QStateMachine machine;
    bool setupCalled = false;
    bool skipPredicateCalled = false;
    bool skipActionCalled = false;
    bool skipHandled = false;

    SkippableAsyncState::SkipAction skipAction;
    if (withSkipAction) {
        skipAction = [&skipActionCalled]() { skipActionCalled = true; };
    }
    auto* skippableState = new SkippableAsyncState(
        QStringLiteral("TestSkippableAsyncSkip"), &machine,
        [&skipPredicateCalled]() {
            skipPredicateCalled = true;
            return true;  // Skip
        },
        [&setupCalled](SkippableAsyncState* state) {
            Q_UNUSED(state);
            setupCalled = true;  // Should NOT be called
        },
        skipAction);
    auto* skipState = new FunctionState(QStringLiteral("SkipHandler"), &machine, [&skipHandled]() {
        skipHandled = true;
    });
    auto* finalState = addFinalState(&machine);

    skippableState->addTransition(skippableState, &QGCState::advance, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::skipped, skipState);
    skipState->addTransition(skipState, &QGCState::advance, finalState);
    machine.setInitialState(skippableState);

    // Use MultiSignalSpy to verify skipped() fired but advance() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(skippableState));

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(skipPredicateCalled);
    QVERIFY(!setupCalled);  // Setup should NOT have been called
    QCOMPARE(skipActionCalled, withSkipAction);
    QVERIFY(skipHandled);
    QVERIFY_SIGNAL_EMITTED(stateSpy, "skipped");
    QVERIFY_NO_SIGNAL(stateSpy, "advance");
}

void SkippableAsyncStateTest::_testSkippableAsyncStateTimeout()
{
    ignoreTimeoutWarnings();
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
    auto* finalState = addFinalState(&machine);

    skippableState->addTransition(skippableState, &QGCState::advance, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::skipped, finalState);
    skippableState->addTransition(skippableState, &SkippableAsyncState::timeout, timeoutState);
    timeoutState->addTransition(timeoutState, &QGCState::advance, finalState);
    machine.setInitialState(skippableState);

    // Use MultiSignalSpy to verify timeout() fired but advance() and skipped() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(skippableState));

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(timeoutReached);
    QVERIFY_SIGNAL_EMITTED(stateSpy, "timeout");
    QVERIFY_NO_SIGNAL(stateSpy, "advance");
    QVERIFY_NO_SIGNAL(stateSpy, "skipped");
}

void SkippableAsyncStateTest::_testConnectToCompletion_data()
{
    QTest::addColumn<bool>("withSlot");

    QTest::newRow("the signal completes the state") << false;
    QTest::newRow("the slot gets the signal and completes the state") << true;
}

void SkippableAsyncStateTest::_testConnectToCompletion()
{
    QFETCH(bool, withSlot);

    QStateMachine machine;
    QObject signalSource;
    QString receivedName;

    auto* skippableState = new SkippableAsyncState(
        QStringLiteral("TestSkippableAsync"), &machine, []() { return false; },
        [&signalSource, &receivedName, withSlot](SkippableAsyncState* state) {
            if (withSlot) {
                state->connectToCompletion(&signalSource, &QObject::objectNameChanged,
                                           [state, &receivedName](const QString& name) {
                                               receivedName = name;
                                               state->complete();
                                           });
            } else {
                state->connectToCompletion(&signalSource, &QObject::objectNameChanged);
            }
        });
    finishOn(machine, skippableState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(startAndWaitForEntered(&machine, skippableState));
    QCOMPARE(finishedSpy.count(), 0);

    signalSource.setObjectName(QStringLiteral("done"));
    QVERIFY(spyTriggered(finishedSpy));
    QCOMPARE(receivedName, withSlot ? QStringLiteral("done") : QString());
}

UT_REGISTER_TEST(SkippableAsyncStateTest, TestLabel::Unit, TestLabel::Utilities)
