#include "QGCStateMachineTest.h"

#include "MultiSignalSpy.h"
#include "QGCStateMachine.h"
#include "WaitStateBase.h"

#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

namespace {
bool _spyTriggered(QSignalSpy& spy, int timeoutMsecs)
{
    return (spy.count() > 0) || spy.wait(timeoutMsecs);
}

float _spyFloatArg(const QSignalSpy& spy, int index)
{
    return spy.at(index).at(0).toFloat();
}
}

/// Minimal concrete implementation of WaitStateBase for testing
class MinimalTestWaitState : public WaitStateBase
{
    Q_OBJECT
public:
    MinimalTestWaitState(const QString& name, QState* parent, int timeoutMsecs = 0)
        : WaitStateBase(name, parent, timeoutMsecs) {}
protected:
    void connectWaitSignal() override {}
    void disconnectWaitSignal() override {}
};

class ErroringAbstractState : public QGCAbstractState
{
public:
    ErroringAbstractState(const QString& name, QState* parent)
        : QGCAbstractState(name, parent) {}

    void triggerError() { emit error(); }
};


void QGCStateMachineTest::_testQGCStateMachineFactories()
{
    QGCStateMachine machine(QStringLiteral("FactoryTest"), nullptr);
    bool functionCalled = false;

    auto* funcState = machine.addFunctionState(QStringLiteral("Func"), [&functionCalled]() {
        functionCalled = true;
    });
    auto* delayState = machine.addDelayState(50);
    auto* finalState = machine.addFinalState(QStringLiteral("Final"));

    funcState->addTransition(funcState, &QGCState::advance, delayState);
    delayState->addTransition(delayState, &DelayState::delayComplete, finalState);

    machine.setInitialState(funcState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(functionCalled);
}

void QGCStateMachineTest::_testGlobalErrorState()
{
    QGCStateMachine machine(QStringLiteral("GlobalErrorTest"), nullptr);
    bool errorHandled = false;

    auto* errorState = new FunctionState(QStringLiteral("GlobalError"), &machine, [&errorHandled]() {
        errorHandled = true;
    });
    machine.setGlobalErrorState(errorState);

    auto* asyncState = machine.addAsyncFunctionState(QStringLiteral("Async"), [](AsyncFunctionState* state) {
        QTimer::singleShot(50, state, [state]() { state->fail(); });
    });
    auto* finalState = machine.addFinalState();

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);

    machine.setInitialState(asyncState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(errorHandled);
}

void QGCStateMachineTest::_testGlobalErrorStateWithAbstractState()
{
    QGCStateMachine machine(QStringLiteral("AbstractGlobalErrorTest"), nullptr);
    auto* errorState = new FunctionState(QStringLiteral("GlobalError"), &machine, []() {});
    machine.setGlobalErrorState(errorState);

    auto* abstractState = new ErroringAbstractState(QStringLiteral("AbstractError"), &machine);
    machine.registerState(abstractState);

    const auto transitions = machine.transitionsFrom(abstractState);
    QVERIFY(!transitions.isEmpty());

    bool foundGlobalErrorTransition = false;
    for (auto* transition : transitions) {
        if (transition && transition->targetState() == errorState) {
            foundGlobalErrorTransition = true;
            break;
        }
    }
    QVERIFY(foundGlobalErrorTransition);

    machine.setInitialState(abstractState);
    QObject::connect(abstractState, &QGCAbstractState::error, &machine, &QStateMachine::stop);
    QSignalSpy enteredSpy(abstractState, &QAbstractState::entered);
    QSignalSpy errorSpy(abstractState, &QGCAbstractState::error);
    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);
    machine.start();

    QVERIFY(enteredSpy.wait(500));
    abstractState->triggerError();
    QVERIFY((errorSpy.count() > 0) || errorSpy.wait(500));
    QVERIFY((stoppedSpy.count() > 0) || stoppedSpy.wait(500));
}

void QGCStateMachineTest::_testLocalErrorState()
{
    QGCStateMachine machine(QStringLiteral("LocalErrorTest"), nullptr);
    bool globalErrorHandled = false;
    bool localErrorHandled = false;

    auto* globalErrorState = new FunctionState(QStringLiteral("GlobalError"), &machine, [&globalErrorHandled]() {
        globalErrorHandled = true;
    });
    machine.setGlobalErrorState(globalErrorState);

    auto* localErrorState = new FunctionState(QStringLiteral("LocalError"), &machine, [&localErrorHandled]() {
        localErrorHandled = true;
    });

    auto* asyncState = new AsyncFunctionState(QStringLiteral("Async"), &machine, [](AsyncFunctionState* state) {
        QTimer::singleShot(50, state, [state]() { state->fail(); });
    });
    asyncState->setLocalErrorState(localErrorState);  // Override global error for this state

    auto* finalState = machine.addFinalState();

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    globalErrorState->addTransition(globalErrorState, &QGCState::advance, finalState);
    localErrorState->addTransition(localErrorState, &QGCState::advance, finalState);

    machine.setInitialState(asyncState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(!globalErrorHandled);  // Global error should NOT be called
    QVERIFY(localErrorHandled);    // Local error should be called
}

void QGCStateMachineTest::_testPropertyAssignment()
{
    QGCStateMachine machine(QStringLiteral("PropTest"), nullptr);
    machine.enablePropertyRestore();

    QObject target;
    target.setProperty("testProp", 0);

    // Use a state that doesn't transition immediately so we can verify property
    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    state1->setProperty(&target, "testProp", 42);

    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* finalState = new QFinalState(&machine);

    // Use event-based transitions instead of entered signal
    machine.addEventTransition(state1, QStringLiteral("next"), state2);
    state2->addTransition(state2, &QState::entered, finalState);

    machine.setInitialState(state1);

    QSignalSpy state1EnteredSpy(state1, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    // Wait for state1 to be entered
    QVERIFY(state1EnteredSpy.wait(500));
    // Property should be set to 42 while in state1
    QCOMPARE(target.property("testProp").toInt(), 42);

    // Trigger transition to state2
    machine.postEvent(QStringLiteral("next"));

    QVERIFY(finishedSpy.wait(500));
    // Property should be restored after leaving state1
    QCOMPARE(target.property("testProp").toInt(), 0);
}

// -----------------------------------------------------------------------------
// Transition Builder Tests
// -----------------------------------------------------------------------------

void QGCStateMachineTest::_testTimeoutTransitionBuilder()
{
    QGCStateMachine machine(QStringLiteral("TimeoutBuilderTest"), nullptr);

    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* finalState = machine.addFinalState();

    // Use the builder to add a timeout transition
    auto* timeout = machine.addTimeoutTransition(state1, 50, state2);
    state2->addTransition(state2, &QState::entered, finalState);

    machine.setInitialState(state1);

    QVERIFY(timeout != nullptr);
    QCOMPARE(timeout->timeoutMsecs(), 50);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
}

void QGCStateMachineTest::_testRetryTransitionBuilder()
{
    QGCStateMachine machine(QStringLiteral("RetryBuilderTest"), nullptr);
    int retryCount = 0;

    auto* state1 = new MinimalTestWaitState(QStringLiteral("State1"), &machine, 50);
    auto* finalState = machine.addFinalState();

    // Use the builder to add a retry transition
    auto* retry = machine.addRetryTransition(state1, &WaitStateBase::timeout, finalState,
                                              [&retryCount]() { retryCount++; }, 2);

    machine.setInitialState(state1);

    QVERIFY(retry != nullptr);
    QCOMPARE(retry->maxRetries(), 2);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(1000));
    // Should have retried twice before advancing
    QCOMPARE(retryCount, 2);
}

void QGCStateMachineTest::_testConditionalTransitionBuilder()
{
    QGCStateMachine machine(QStringLiteral("ConditionalBuilderTest"), nullptr);
    bool guardResult = false;

    auto* state1 = new FunctionState(QStringLiteral("State1"), &machine, []() {});
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* state3 = new QGCState(QStringLiteral("State3"), &machine);
    auto* finalState = machine.addFinalState();

    // Add conditional transition with guard
    machine.addConditionalTransition(state1, &FunctionState::advance, state2,
                                      [&guardResult]() { return guardResult; });
    // Add fallback unguarded transition
    state1->addTransition(state1, &FunctionState::advance, state3);
    state2->addTransition(state2, &QState::entered, finalState);
    state3->addTransition(state3, &QState::entered, finalState);

    machine.setInitialState(state1);

    // First run: guard is false, should go to state3
    QSignalSpy state3EnteredSpy(state3, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(state3EnteredSpy.count(), 1);
}

void QGCStateMachineTest::_testErrorRecoveryFactory()
{
    QGCStateMachine machine(QStringLiteral("ErrorRecoveryFactoryTest"), nullptr);
    int actionCalls = 0;

    auto* recoveryState = machine.addErrorRecoveryState(
        QStringLiteral("Recover"),
        [&actionCalls]() {
            actionCalls++;
            return false;
        },
        1,
        1,
        ErrorRecoveryBuilder::EmitAdvance
    );
    auto* finalState = machine.addFinalState();

    recoveryState->addTransition(recoveryState, &QGCState::advance, finalState);
    recoveryState->addTransition(recoveryState, &QGCState::error, finalState);
    machine.setInitialState(recoveryState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(_spyTriggered(finishedSpy, 1000));
    QCOMPARE(actionCalls, 2);
}

void QGCStateMachineTest::_testErrorHandlerFactories()
{
    {
        QGCStateMachine machine(QStringLiteral("ErrorHandlerContinueFactoryTest"), nullptr);

        auto* finalState = machine.addFinalState();
        auto* errorState = machine.addLogAndContinueErrorState(QStringLiteral("ErrorContinue"), finalState);
        machine.setGlobalErrorState(errorState);

        auto* failingState = machine.addAsyncFunctionState(QStringLiteral("Failing"), [](AsyncFunctionState* state) {
            QTimer::singleShot(0, state, [state]() { state->fail(); });
        });

        machine.setInitialState(failingState);

        QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
        machine.start();
        QVERIFY(_spyTriggered(finishedSpy, 500));
    }

    {
        QGCStateMachine machine(QStringLiteral("ErrorHandlerStopFactoryTest"), nullptr);

        auto* errorState = machine.addLogAndStopErrorState(QStringLiteral("ErrorStop"));
        machine.setGlobalErrorState(errorState);

        auto* failingState = machine.addAsyncFunctionState(QStringLiteral("Failing"), [](AsyncFunctionState* state) {
            QTimer::singleShot(0, state, [state]() { state->fail(); });
        });

        machine.setInitialState(failingState);

        QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);
        machine.start();
        QVERIFY(_spyTriggered(stoppedSpy, 500));
    }
}

// -----------------------------------------------------------------------------
// State Introspection Tests
// -----------------------------------------------------------------------------

void QGCStateMachineTest::_testTransitionsFrom()
{
    QGCStateMachine machine(QStringLiteral("IntrospectTest"), nullptr);

    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* state3 = new QGCState(QStringLiteral("State3"), &machine);

    state1->addTransition(state1, &QGCState::advance, state2);
    state1->addTransition(state1, &QGCState::error, state3);

    auto transitions = machine.transitionsFrom(state1);
    QCOMPARE(transitions.count(), 2);

    // State2 has no outgoing transitions
    QCOMPARE(machine.transitionsFrom(state2).count(), 0);
}

void QGCStateMachineTest::_testTransitionsTo()
{
    QGCStateMachine machine(QStringLiteral("IntrospectTest"), nullptr);

    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* state3 = new QGCState(QStringLiteral("State3"), &machine);

    state1->addTransition(state1, &QGCState::advance, state3);
    state2->addTransition(state2, &QGCState::advance, state3);

    // State3 is targeted by two transitions
    auto transitions = machine.transitionsTo(state3);
    QCOMPARE(transitions.count(), 2);

    // State1 is not targeted by any transition
    QCOMPARE(machine.transitionsTo(state1).count(), 0);
}

void QGCStateMachineTest::_testReachableFrom()
{
    QGCStateMachine machine(QStringLiteral("IntrospectTest"), nullptr);

    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* state3 = new QGCState(QStringLiteral("State3"), &machine);

    state1->addTransition(state1, &QGCState::advance, state2);
    state1->addTransition(state1, &QGCState::error, state3);

    auto reachable = machine.reachableFrom(state1);
    QCOMPARE(reachable.count(), 2);
    QVERIFY(reachable.contains(state2));
    QVERIFY(reachable.contains(state3));
}

void QGCStateMachineTest::_testPredecessorsOf()
{
    QGCStateMachine machine(QStringLiteral("IntrospectTest"), nullptr);

    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* state3 = new QGCState(QStringLiteral("State3"), &machine);

    state1->addTransition(state1, &QGCState::advance, state3);
    state2->addTransition(state2, &QGCState::advance, state3);

    auto predecessors = machine.predecessorsOf(state3);
    QCOMPARE(predecessors.count(), 2);
    QVERIFY(predecessors.contains(state1));
    QVERIFY(predecessors.contains(state2));
}

void QGCStateMachineTest::_testProgressTrackingWithSubProgress()
{
    QGCStateMachine machine(QStringLiteral("ProgressTrackingTest"), nullptr);

    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* finalState = machine.addFinalState();

    machine.addEventTransition(state1, QStringLiteral("toState2"), state2);
    machine.addEventTransition(state2, QStringLiteral("finish"), finalState);
    machine.setProgressWeights({{state1, 1}, {state2, 3}});
    machine.setInitialState(state1);

    QSignalSpy progressSpy(&machine, &QGCStateMachine::progressUpdate);
    QSignalSpy state1EnteredSpy(state1, &QState::entered);
    QSignalSpy state2EnteredSpy(state2, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(_spyTriggered(state1EnteredSpy, 500));

    machine.setSubProgress(0.5f);
    machine.postEvent(QStringLiteral("toState2"));
    QVERIFY(_spyTriggered(state2EnteredSpy, 500));

    machine.setSubProgress(0.5f);
    machine.setSubProgress(0.4f); // lower value should not emit
    machine.setSubProgress(1.0f);

    machine.postEvent(QStringLiteral("finish"));
    QVERIFY(_spyTriggered(finishedSpy, 500));

    QCOMPARE(progressSpy.count(), 4);
    QVERIFY(qAbs(_spyFloatArg(progressSpy, 0) - 0.125f) < 0.001f);
    QVERIFY(qAbs(_spyFloatArg(progressSpy, 1) - 0.25f) < 0.001f);
    QVERIFY(qAbs(_spyFloatArg(progressSpy, 2) - 0.625f) < 0.001f);
    QVERIFY(qAbs(_spyFloatArg(progressSpy, 3) - 1.0f) < 0.001f);
    QVERIFY(qAbs(machine.progress() - 1.0f) < 0.001f);
}

void QGCStateMachineTest::_testStateHistoryLimit()
{
    QGCStateMachine machine(QStringLiteral("StateHistoryLimitTest"), nullptr);

    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* state3 = new QGCState(QStringLiteral("State3"), &machine);
    auto* finalState = machine.addFinalState();

    machine.addEventTransition(state1, QStringLiteral("toState2"), state2);
    machine.addEventTransition(state2, QStringLiteral("toState3"), state3);
    machine.addEventTransition(state3, QStringLiteral("finish"), finalState);
    machine.setStateHistoryLimit(2);
    machine.setProgressWeights({{state1, 1}, {state2, 1}, {state3, 1}});
    machine.setInitialState(state1);

    QSignalSpy state1EnteredSpy(state1, &QState::entered);
    QSignalSpy state2EnteredSpy(state2, &QState::entered);
    QSignalSpy state3EnteredSpy(state3, &QState::entered);
    QSignalSpy historySpy(&machine, &QGCStateMachine::stateHistoryChanged);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(_spyTriggered(state1EnteredSpy, 500));
    machine.postEvent(QStringLiteral("toState2"));
    QVERIFY(_spyTriggered(state2EnteredSpy, 500));
    machine.postEvent(QStringLiteral("toState3"));
    QVERIFY(_spyTriggered(state3EnteredSpy, 500));

    const QStringList history = machine.stateHistory();
    QCOMPARE(history.size(), 2);
    QCOMPARE(history.at(0), QStringLiteral("State2"));
    QCOMPARE(history.at(1), QStringLiteral("State3"));
    QVERIFY(historySpy.count() >= 3);
    QCOMPARE(machine.currentStateName(), QStringLiteral("State3"));

    machine.postEvent(QStringLiteral("finish"));
    QVERIFY(_spyTriggered(finishedSpy, 500));
}

void QGCStateMachineTest::_testRuntimeDiagnosticsHelpers()
{
    QGCStateMachine machine(QStringLiteral("RuntimeDiagnosticsHelpersTest"), nullptr);

    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    auto* finalState = machine.addFinalState();

    machine.addEventTransition(state1, QStringLiteral("next"), state2);
    machine.addEventTransition(state2, QStringLiteral("finish"), finalState);
    machine.setInitialState(state1);

    machine.setHistoryRecordingEnabled(true, 32);
    machine.setProfilingEnabled(true);
    machine.setStructuredLoggingEnabled(true);

    QVERIFY(machine.historyRecordingEnabled());
    QVERIFY(machine.profilingEnabled());
    QVERIFY(machine.structuredLoggingEnabled());

    QSignalSpy state1EnteredSpy(state1, &QState::entered);
    QSignalSpy state2EnteredSpy(state2, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(_spyTriggered(state1EnteredSpy, 500));
    machine.postEvent(QStringLiteral("next"));
    QVERIFY(_spyTriggered(state2EnteredSpy, 500));
    machine.postEvent(QStringLiteral("finish"));
    QVERIFY(_spyTriggered(finishedSpy, 500));

    QVERIFY(!machine.dumpRecordedHistory().isEmpty());
    QVERIFY(machine.recordedHistoryJson().size() >= 2);
    QVERIFY(!machine.profilingSummary().isEmpty());

    machine.setHistoryRecordingEnabled(false);
    machine.setProfilingEnabled(false);
    machine.setStructuredLoggingEnabled(false);

    QVERIFY(!machine.historyRecordingEnabled());
    QVERIFY(!machine.profilingEnabled());
    QVERIFY(!machine.structuredLoggingEnabled());
}

// -----------------------------------------------------------------------------
// Composite State Pattern Tests
// -----------------------------------------------------------------------------

void QGCStateMachineTest::_testTimedActionState()
{
    QGCStateMachine machine(QStringLiteral("TimedActionTest"), nullptr);

    // Create a timed state that waits 50ms
    auto* timedState = machine.createTimedActionState(QStringLiteral("Wait50ms"), 50);
    auto* finalState = machine.addFinalState();

    // Use QState::finished to transition (composite state pattern)
    timedState->addTransition(timedState, &QState::finished, finalState);

    machine.setInitialState(timedState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    // Should finish after ~50ms
    QVERIFY(finishedSpy.wait(500));
}

void QGCStateMachineTest::_testTimedActionStateWithCallbacks()
{
    QGCStateMachine machine(QStringLiteral("TimedActionCallbackTest"), nullptr);
    bool entryActionCalled = false;
    bool exitActionCalled = false;

    // Create a timed state with entry and exit callbacks
    auto* timedState = machine.createTimedActionState(
        QStringLiteral("WaitWithCallbacks"),
        50,
        [&entryActionCalled]() { entryActionCalled = true; },
        [&exitActionCalled]() { exitActionCalled = true; }
    );
    auto* finalState = machine.addFinalState();

    timedState->addTransition(timedState, &QState::finished, finalState);

    machine.setInitialState(timedState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(entryActionCalled);
    QVERIFY(exitActionCalled);
}

void QGCStateMachineTest::_testSelfLoopTransition()
{
    QGCStateMachine machine(QStringLiteral("SelfLoopTest"), nullptr);
    int actionCount = 0;

    // Create a state that stays active and handles signals
    auto* loopState = new QGCState(QStringLiteral("LoopState"), &machine);
    auto* finalState = machine.addFinalState();

    // Create a signal source
    QObject signalSource;

    // Add self-loop transition that increments counter
    machine.addSelfLoopTransition(loopState, &signalSource, &QObject::objectNameChanged,
                                   [&actionCount]() { actionCount++; });

    // Also add exit transition after 3 loops (use machine event)
    machine.addEventTransition(loopState, QStringLiteral("done"), finalState);

    machine.setInitialState(loopState);

    QSignalSpy enteredSpy(loopState, &QState::entered);
    machine.start();

    QVERIFY(enteredSpy.wait(500));

    // Trigger the self-loop 3 times
    signalSource.setObjectName(QStringLiteral("1"));
    signalSource.setObjectName(QStringLiteral("2"));
    signalSource.setObjectName(QStringLiteral("3"));

    // Process events to ensure actions are called
    QCoreApplication::processEvents();

    QCOMPARE(actionCount, 3);

    // Now exit
    machine.postEvent(QStringLiteral("done"));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(finishedSpy.wait(500));
}

void QGCStateMachineTest::_testInternalTransition()
{
    QGCStateMachine machine(QStringLiteral("InternalTransitionTest"), nullptr);
    int entryCount = 0;
    int actionCount = 0;

    // Create a state
    auto* state = new QGCState(QStringLiteral("State"), &machine);
    state->setOnEntry([&entryCount]() { entryCount++; });

    auto* finalState = machine.addFinalState();

    // Create a signal source
    QObject signalSource;

    // Add internal transition (should NOT re-trigger entry)
    machine.addInternalTransition(state, &signalSource, &QObject::objectNameChanged,
                                   [&actionCount]() { actionCount++; });

    // Exit transition
    machine.addEventTransition(state, QStringLiteral("done"), finalState);

    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QState::entered);
    machine.start();

    QVERIFY(enteredSpy.wait(500));
    QCOMPARE(entryCount, 1);  // Entered once

    // Trigger internal transition
    signalSource.setObjectName(QStringLiteral("test"));
    QCoreApplication::processEvents();

    QCOMPARE(actionCount, 1);   // Action was called
    QCOMPARE(entryCount, 1);    // Entry was NOT called again (internal transition)

    // Exit
    machine.postEvent(QStringLiteral("done"));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(finishedSpy.wait(500));
}

#include "QGCStateMachineTest.moc"

UT_REGISTER_TEST(QGCStateMachineTest, TestLabel::Unit, TestLabel::Utilities)
