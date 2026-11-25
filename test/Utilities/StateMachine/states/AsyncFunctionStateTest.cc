#include "AsyncFunctionStateTest.h"
#include "StateTestCommon.h"

#include <QtCore/QRegularExpression>


void AsyncFunctionStateTest::_testAsyncFunctionState()
{
    QStateMachine machine;
    bool setupCalled = false;
    AsyncFunctionState* capturedState = nullptr;

    // Expected: async state has no completion connection and no timeout
    expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtCriticalMsg,
                     QRegularExpression("has no completion connection"));

    auto* asyncState = new AsyncFunctionState(
        QStringLiteral("TestAsync"),
        &machine,
        [&setupCalled, &capturedState](AsyncFunctionState* state) {
            setupCalled = true;
            capturedState = state;
            QTimer::singleShot(50, state, [state]() {
                state->complete();
            });
        }
    );
    QVERIFY(runStateToCompletion(asyncState, &machine));
    verifyExpectedLogMessage();
    QVERIFY(setupCalled);
    QVERIFY(capturedState != nullptr);
}

void AsyncFunctionStateTest::_testAsyncFunctionStateTimeout()
{
    ignoreTimeoutWarnings();
    QStateMachine machine;
    bool timeoutReached = false;
    const int timeoutMs = 100;

    auto* asyncState = new AsyncFunctionState(
        QStringLiteral("TestAsyncTimeout"),
        &machine,
        [](AsyncFunctionState* state) {
            Q_UNUSED(state);
            // Don't call complete() - let it timeout
        },
        timeoutMs
    );
    auto* timeoutState = new FunctionState(QStringLiteral("TimeoutHandler"), &machine, [&timeoutReached]() {
        timeoutReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    asyncState->addTransition(asyncState, &AsyncFunctionState::timedOut, timeoutState);
    timeoutState->addTransition(timeoutState, &QGCState::advance, finalState);
    machine.setInitialState(asyncState);

    // Use MultiSignalSpy to verify timedOut() fired but advance() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(asyncState));

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(timeoutReached);
    QVERIFY_SIGNAL_EMITTED(stateSpy, "timedOut");
    QVERIFY_NO_SIGNAL(stateSpy, "advance");
}

void AsyncFunctionStateTest::_testErrorTransition()
{
    QStateMachine machine;
    bool errorHandled = false;

    // Expected: async state has no completion connection and no timeout
    expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtCriticalMsg,
                     QRegularExpression("has no completion connection"));

    auto* asyncState = new AsyncFunctionState(
        QStringLiteral("TestError"),
        &machine,
        [](AsyncFunctionState* state) {
            QTimer::singleShot(50, state, [state]() {
                state->fail();
            });
        }
    );
    auto* errorState = new FunctionState(QStringLiteral("ErrorHandler"), &machine, [&errorHandled]() {
        errorHandled = true;
    });
    auto* finalState = new QFinalState(&machine);

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    asyncState->addTransition(asyncState, &QGCState::error, errorState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);
    machine.setInitialState(asyncState);

    // Use MultiSignalSpy to verify error() fired but advance() did not
    MultiSignalSpy stateSpy;
    QVERIFY(stateSpy.init(asyncState));

    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QVERIFY(errorHandled);
    QVERIFY_SIGNAL_EMITTED(stateSpy, "error");
    QVERIFY_NO_SIGNAL(stateSpy, "advance");
}

void AsyncFunctionStateTest::_testSetupFunctionEndsState_data()
{
    QTest::addColumn<bool>("fails");

    QTest::newRow("the setup function completes the state") << false;
    QTest::newRow("the setup function fails the state") << true;
}

void AsyncFunctionStateTest::_testSetupFunctionEndsState()
{
    QFETCH(bool, fails);

    QStateMachine machine;
    // Without a completion connection and without a timeout, which is reported only for a state that waits
    auto* asyncState =
        new AsyncFunctionState(QStringLiteral("TestAsync"), &machine, [fails](AsyncFunctionState* state) {
            if (fails) {
                state->fail();
            } else {
                state->complete();
            }
        });
    finishOn(machine, asyncState, fails ? &QGCState::error : &QGCState::advance);

    QVERIFY(startAndWaitForFinished(&machine));
}

void AsyncFunctionStateTest::_testConnectToCompletion_data()
{
    QTest::addColumn<bool>("withSlot");

    QTest::newRow("the signal completes the state") << false;
    QTest::newRow("the slot gets the signal and completes the state") << true;
}

void AsyncFunctionStateTest::_testConnectToCompletion()
{
    QFETCH(bool, withSlot);

    QStateMachine machine;
    QObject signalSource;
    QString receivedName;

    auto* asyncState = new AsyncFunctionState(
        QStringLiteral("TestAsync"), &machine, [&signalSource, &receivedName, withSlot](AsyncFunctionState* state) {
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
    finishOn(machine, asyncState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(startAndWaitForEntered(&machine, asyncState));
    QCOMPARE(finishedSpy.count(), 0);

    signalSource.setObjectName(QStringLiteral("done"));
    QVERIFY(spyTriggered(finishedSpy));
    QCOMPARE(receivedName, withSlot ? QStringLiteral("done") : QString());
}

void AsyncFunctionStateTest::_testEveryCompletionConnectionEndsWithWait()
{
    QStateMachine machine;
    QObject firstSource;
    QObject secondSource;
    int slotCalls = 0;

    auto* asyncState = new AsyncFunctionState(
        QStringLiteral("TestAsync"), &machine, [&firstSource, &secondSource, &slotCalls](AsyncFunctionState* state) {
            state->connectToCompletion(&firstSource, &QObject::objectNameChanged, [state, &slotCalls](const QString&) {
                slotCalls++;
                state->complete();
            });
            state->connectToCompletion(&secondSource, &QObject::objectNameChanged);
        });
    finishOn(machine, asyncState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(startAndWaitForEntered(&machine, asyncState));
    secondSource.setObjectName(QStringLiteral("done"));
    QVERIFY(spyTriggered(finishedSpy));

    // The wait has ended, so the slot of the connection that did not end it is not called any more
    firstSource.setObjectName(QStringLiteral("late"));
    QCOMPARE(slotCalls, 0);
}

void AsyncFunctionStateTest::_testConnectionWhileWaitingEndsWithWait()
{
    QStateMachine machine;
    QObject progressSource;
    QObject doneSource;
    int progressCalls = 0;

    auto* asyncState =
        new AsyncFunctionState(QStringLiteral("TestAsync"), &machine,
                               [&progressSource, &doneSource, &progressCalls](AsyncFunctionState* state) {
                                   state->connectWhileWaiting(&progressSource, &QObject::objectNameChanged,
                                                              [&progressCalls](const QString&) { progressCalls++; });
                                   state->connectToCompletion(&doneSource, &QObject::objectNameChanged);
                               });
    finishOn(machine, asyncState, &QGCState::advance);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(startAndWaitForEntered(&machine, asyncState));

    // The slot is called while the state waits, and the state goes on waiting
    progressSource.setObjectName(QStringLiteral("half way"));
    QCOMPARE(progressCalls, 1);
    QCOMPARE(finishedSpy.count(), 0);

    doneSource.setObjectName(QStringLiteral("done"));
    QVERIFY(spyTriggered(finishedSpy));
    progressSource.setObjectName(QStringLiteral("late"));
    QCOMPARE(progressCalls, 1);
}

void AsyncFunctionStateTest::_testConnectionWhileWaitingDoesNotEndWait()
{
    QStateMachine machine;
    QObject progressSource;

    // Without a timeout, and with a connection that cannot end the wait, nothing ends the state
    auto* asyncState =
        new AsyncFunctionState(QStringLiteral("TestAsync"), &machine, [&progressSource](AsyncFunctionState* state) {
            state->connectWhileWaiting(&progressSource, &QObject::objectNameChanged, [](const QString&) {});
        });
    machine.setInitialState(asyncState);

    expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtCriticalMsg,
                     QRegularExpression("has no completion connection"));
    QVERIFY(startAndWaitForEntered(&machine, asyncState));
    verifyExpectedLogMessage();
}

void AsyncFunctionStateTest::_testSkipPredicate_data()
{
    QTest::addColumn<bool>("skip");
    QTest::addColumn<bool>("withSkipAction");

    QTest::newRow("the predicate says no") << false << true;
    QTest::newRow("skipped, without a skip action") << true << false;
    QTest::newRow("skipped, with a skip action") << true << true;
}

void AsyncFunctionStateTest::_testSkipPredicate()
{
    QFETCH(bool, skip);
    QFETCH(bool, withSkipAction);

    QStateMachine machine;
    bool setupCalled = false;
    bool skipActionCalled = false;

    auto* asyncState =
        new AsyncFunctionState(QStringLiteral("TestSkip"), &machine, [&setupCalled](AsyncFunctionState* state) {
            setupCalled = true;
            state->complete();
        });
    AsyncFunctionState::SkipAction skipAction;
    if (withSkipAction) {
        skipAction = [&skipActionCalled]() { skipActionCalled = true; };
    }
    asyncState->setSkipPredicate([skip]() { return skip; }, skipAction);
    // A skipped step leaves the state as a completed one does
    finishOn(machine, asyncState, &QGCState::advance);

    QVERIFY(startAndWaitForFinished(&machine));
    QCOMPARE(setupCalled, !skip);
    QCOMPARE(skipActionCalled, skip && withSkipAction);
}

void AsyncFunctionStateTest::_testSkippedBeforeMachineStops()
{
    QGCStateMachine machine(QStringLiteral("SkippedBeforeStoppedTest"), nullptr);
    QStringList signalOrder;

    // The skip action stops the machine while the state is entered
    auto* asyncState = new AsyncFunctionState(QStringLiteral("Skipped"), &machine, [](AsyncFunctionState*) {});
    asyncState->setSkipPredicate([]() { return true; }, [&machine]() { machine.stop(); });
    machine.setInitialState(asyncState);
    connect(asyncState, &QGCState::advance, &machine,
            [&signalOrder]() { signalOrder.append(QStringLiteral("advance")); });
    connect(&machine, &QStateMachine::stopped, &machine,
            [&signalOrder]() { signalOrder.append(QStringLiteral("stopped")); });

    machine.start();
    QTRY_COMPARE_WITH_TIMEOUT(signalOrder.size(), 2, TestTimeout::shortMs());
    // The state does not report that it was skipped to a machine that has stopped
    QCOMPARE(signalOrder, QStringList({QStringLiteral("advance"), QStringLiteral("stopped")}));
}

UT_REGISTER_TEST(AsyncFunctionStateTest, TestLabel::Unit, TestLabel::Utilities)
