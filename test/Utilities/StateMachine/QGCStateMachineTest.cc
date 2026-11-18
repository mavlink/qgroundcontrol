#include "QGCStateMachineTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "QGCStateMachine.h"
#include "StateTestCommon.h"

namespace {
float _spyFloatArg(const QSignalSpy& spy, int index)
{
    return spy.at(index).at(0).toFloat();
}
}

void QGCStateMachineTest::_testQGCStateMachineFactories()
{
    QGCStateMachine machine(QStringLiteral("FactoryTest"), nullptr);
    QCOMPARE(machine.machineName(), QStringLiteral("FactoryTest"));
    bool functionCalled = false;

    auto* funcState = machine.addFunctionState(QStringLiteral("Func"), [&functionCalled]() {
        functionCalled = true;
    });
    auto* delayState = machine.addDelayState(50);
    auto* finalState = machine.addFinalState(QStringLiteral("Final"));

    funcState->addTransition(funcState, &QGCState::advance, delayState);
    delayState->addTransition(delayState, &DelayState::delayComplete, finalState);

    machine.setInitialState(funcState);

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(functionCalled);
}

void QGCStateMachineTest::_testGlobalErrorState()
{
    QGCStateMachine machine(QStringLiteral("GlobalErrorTest"), nullptr);
    bool errorHandled = false;

    auto* errorState = new FunctionState(QStringLiteral("GlobalError"), &machine, [&errorHandled]() {
        errorHandled = true;
    });
    QCOMPARE(machine.globalErrorState(), nullptr);
    machine.setGlobalErrorState(errorState);
    QCOMPARE(machine.globalErrorState(), errorState);

    // The async state has no completion connection and no timeout — expected critical
    expectLogMessage("Utilities.QGCStateMachine", QtCriticalMsg, QRegularExpression("has no completion connection"));

    auto* asyncState = machine.addAsyncFunctionState(QStringLiteral("Async"), [](AsyncFunctionState* state) {
        QTimer::singleShot(50, state, [state]() { state->fail(); });
    });
    auto* finalState = machine.addFinalState();

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);

    machine.setInitialState(asyncState);

    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QVERIFY(errorHandled);
}

void QGCStateMachineTest::_testGlobalErrorStateWithAbstractState()
{
    QGCStateMachine machine(QStringLiteral("AbstractGlobalErrorTest"), nullptr);
    auto* errorState = new FunctionState(QStringLiteral("GlobalError"), &machine, []() {});
    machine.setGlobalErrorState(errorState);

    auto* abstractState = new TestAbstractState(QStringLiteral("AbstractError"), &machine);
    machine.registerState(abstractState);

    const auto transitions = abstractState->transitions();
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
    QSignalSpy errorSpy(abstractState, &QGCAbstractState::error);
    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);
    QVERIFY(startAndWaitForEntered(&machine, abstractState));
    abstractState->emitError();
    QVERIFY((errorSpy.count() > 0) || errorSpy.wait(TestTimeout::shortMs()));
    QVERIFY((stoppedSpy.count() > 0) || stoppedSpy.wait(TestTimeout::shortMs()));
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

    // The async state has no completion connection and no timeout — expected critical
    expectLogMessage("Utilities.QGCStateMachine", QtCriticalMsg, QRegularExpression("has no completion connection"));

    auto* asyncState = new AsyncFunctionState(QStringLiteral("Async"), &machine, [](AsyncFunctionState* state) {
        QTimer::singleShot(50, state, [state]() { state->fail(); });
    });
    asyncState->setLocalErrorState(localErrorState);  // Override global error for this state

    // A state that has its own error state when it is registered gets no transition to the global one
    machine.registerState(asyncState);
    const auto transitions = asyncState->transitions();
    for (const QAbstractTransition* transition : transitions) {
        QVERIFY(transition->targetState() != globalErrorState);
    }

    auto* finalState = machine.addFinalState();

    asyncState->addTransition(asyncState, &QGCState::advance, finalState);
    globalErrorState->addTransition(globalErrorState, &QGCState::advance, finalState);
    localErrorState->addTransition(localErrorState, &QGCState::advance, finalState);

    machine.setInitialState(asyncState);

    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    QVERIFY(!globalErrorHandled);  // Global error should NOT be called
    QVERIFY(localErrorHandled);    // Local error should be called
}

void QGCStateMachineTest::_testLocalErrorStateSetLast_data()
{
    // Names of the error states given to setLocalErrorState(), an empty name for none
    QTest::addColumn<QStringList>("beforeRegistered");
    QTest::addColumn<QStringList>("afterRegistered");
    QTest::addColumn<QString>("expectedErrorState");

    QTest::newRow("set after the state is registered")
        << QStringList() << QStringList{QStringLiteral("Local")} << QStringLiteral("Local");
    QTest::newRow("set before and again after the state is registered")
        << QStringList{QStringLiteral("Local")} << QStringList{QStringLiteral("OtherLocal")}
        << QStringLiteral("OtherLocal");
    QTest::newRow("set, and removed after the state is registered")
        << QStringList{QStringLiteral("Local")} << QStringList{QString()} << QStringLiteral("Global");
}

void QGCStateMachineTest::_testLocalErrorStateSetLast()
{
    QFETCH(QStringList, beforeRegistered);
    QFETCH(QStringList, afterRegistered);
    QFETCH(QString, expectedErrorState);

    QGCStateMachine machine(QStringLiteral("LocalErrorStateSetLastTest"), nullptr);
    QStringList enteredErrorStates;
    QHash<QString, QAbstractState*> errorStates;
    auto* finalState = machine.addFinalState();
    for (const QString& name : {QStringLiteral("Global"), QStringLiteral("Local"), QStringLiteral("OtherLocal")}) {
        auto* errorState =
            new FunctionState(name, &machine, [&enteredErrorStates, name]() { enteredErrorStates.append(name); });
        errorState->addTransition(errorState, &QGCState::advance, finalState);
        errorStates.insert(name, errorState);
    }
    machine.setGlobalErrorState(errorStates.value(QStringLiteral("Global")));

    auto* failingState =
        new AsyncFunctionState(QStringLiteral("Failing"), &machine, [](AsyncFunctionState* state) { state->fail(); });
    for (const QString& name : beforeRegistered) {
        failingState->setLocalErrorState(errorStates.value(name));
    }
    // As the factories of the machine do for the states they create
    machine.registerState(failingState);
    for (const QString& name : afterRegistered) {
        failingState->setLocalErrorState(errorStates.value(name));
    }
    machine.setInitialState(failingState);

    QVERIFY(startAndWaitForFinished(&machine));
    QCOMPARE(enteredErrorStates, QStringList{expectedErrorState});
}

void QGCStateMachineTest::_testInProgressSpansQueuedStart_data()
{
    QTest::addColumn<bool>("hasInitialState");
    QTest::addColumn<bool>("stopBeforeStartRuns");

    QTest::newRow("runsToFinish") << true << false;
    QTest::newRow("stoppedBeforeStartRuns") << true << true;
    QTest::newRow("refusedWithoutInitialState") << false << false;
}

void QGCStateMachineTest::_testInProgressSpansQueuedStart()
{
    QFETCH(bool, hasInitialState);
    QFETCH(bool, stopBeforeStartRuns);

    QGCStateMachine machine(QStringLiteral("InProgressSpansQueuedStartTest"), nullptr);
    auto* state = new QGCState(QStringLiteral("State"), &machine);
    auto* finalState = machine.addFinalState();
    machine.addEventTransition(state, QStringLiteral("finish"), finalState);
    QSignalSpy inProgressSpy(&machine, &QGCStateMachine::inProgressChanged);

    if (!hasInitialState) {
        expectLogMessage("Utilities.QGCStateMachine", QtCriticalMsg,
                         QRegularExpression(QStringLiteral("start\\(\\) called without an initial state")));
        machine.start();
        verifyExpectedLogMessage();
        QVERIFY(!machine.inProgress());
        QCOMPARE(inProgressSpy.count(), 0);
        return;
    }

    machine.setInitialState(state);
    QSignalSpy startedSpy(&machine, &QStateMachine::started);
    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    // Listeners must already see the machine as not in progress, e.g. to restart it or treat it as complete
    int listenerCalls = 0;
    bool inProgressInListener = true;
    const auto recordInProgress = [&machine, &listenerCalls, &inProgressInListener]() {
        listenerCalls++;
        inProgressInListener = machine.inProgress();
    };
    (void) connect(&machine, &QStateMachine::stopped, this, recordInProgress);
    (void) connect(&machine, &QStateMachine::finished, this, recordInProgress);

    machine.start();
    QVERIFY(!machine.QStateMachine::isRunning());
    QVERIFY(machine.inProgress());
    QCOMPARE(inProgressSpy.count(), 1);

    if (stopBeforeStartRuns) {
        machine.stop();
        QVERIFY(machine.inProgress());
    } else {
        QVERIFY(spyTriggered(startedSpy));
        QVERIFY(machine.inProgress());
        machine.postEvent(QStringLiteral("finish"));
    }

    QVERIFY(spyTriggered(stopBeforeStartRuns ? stoppedSpy : finishedSpy));
    QCOMPARE(listenerCalls, 1);
    QVERIFY(!inProgressInListener);
    QVERIFY(!machine.inProgress());
    QCOMPARE(inProgressSpy.count(), 2);
}

void QGCStateMachineTest::_testStopFromInProgressListenerIsHonored()
{
    QGCStateMachine machine(QStringLiteral("StopFromInProgressListenerTest"), nullptr);
    auto* state = new QGCState(QStringLiteral("State"), &machine);
    machine.setInitialState(state);
    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);

    (void) connect(&machine, &QGCStateMachine::inProgressChanged, this, [&machine]() {
        if (machine.inProgress()) {
            machine.stop();
        }
    });

    machine.start();

    QVERIFY(spyTriggered(stoppedSpy));
    QVERIFY(!machine.inProgress());
}

void QGCStateMachineTest::_testStartWhileInProgressIsCritical()
{
    QGCStateMachine machine(QStringLiteral("StartWhileInProgressTest"), nullptr);
    auto* state = new QGCState(QStringLiteral("State"), &machine);
    auto* finalState = machine.addFinalState();
    machine.addEventTransition(state, QStringLiteral("finish"), finalState);
    machine.setInitialState(state);
    QSignalSpy startedSpy(&machine, &QStateMachine::started);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    expectLogMessage("Utilities.QGCStateMachine", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("already in progress")));
    machine.start();
    verifyExpectedLogMessage();

    QVERIFY(spyTriggered(startedSpy));
    machine.postEvent(QStringLiteral("finish"));
    QVERIFY(spyTriggered(finishedSpy));
    QCOMPARE(startedSpy.count(), 1);
    QVERIFY(!machine.inProgress());
}

void QGCStateMachineTest::_testStartBypassIsCritical()
{
    QGCStateMachine machine(QStringLiteral("StartBypassTest"), nullptr);
    auto* state = new QGCState(QStringLiteral("State"), &machine);
    auto* finalState = machine.addFinalState();
    machine.addEventTransition(state, QStringLiteral("finish"), finalState);
    machine.setInitialState(state);
    QSignalSpy startedSpy(&machine, &QStateMachine::started);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    expectLogMessage("Utilities.QGCStateMachine", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("started without QGCStateMachine::start\\(\\)")));
    static_cast<QStateMachine&>(machine).start();
    QVERIFY(!machine.inProgress());
    QVERIFY(spyTriggered(startedSpy));
    verifyExpectedLogMessage();
    QVERIFY(machine.inProgress());

    machine.postEvent(QStringLiteral("finish"));
    QVERIFY(spyTriggered(finishedSpy));
    QVERIFY(!machine.inProgress());
}

void QGCStateMachineTest::_testSetInitialStateStartsMachine()
{
    QGCStateMachine machine(QStringLiteral("AutoStartTest"), nullptr);
    auto* state = new QGCState(QStringLiteral("State"), &machine);
    QSignalSpy enteredSpy(state, &QState::entered);

    machine.setInitialState(state, true);

    QCOMPARE(machine.initialState(), state);
    QVERIFY(machine.inProgress());
    QVERIFY(spyTriggered(enteredSpy));
}

void QGCStateMachineTest::_testErrorInMachineIsCritical()
{
    QGCStateMachine machine(QStringLiteral("MachineErrorTest"), nullptr);
    // Qt cannot enter a state that has child states and no initial state, and stops the machine
    auto* compoundState = new QGCState(QStringLiteral("Compound"), &machine);
    auto* childState = new QGCState(QStringLiteral("Child"), compoundState);
    machine.setInitialState(compoundState);

    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);
    expectLogMessage("default", QtWarningMsg,
                     QRegularExpression("Unrecoverable error detected in running state machine"));
    expectLogMessage(
        "Utilities.QGCStateMachine", QtCriticalMsg,
        QRegularExpression("\"MachineErrorTest\" stopped after an error in the machine: \"Missing initial state"));
    machine.start();
    QVERIFY(spyTriggered(stoppedSpy));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!machine.inProgress());
    QCOMPARE(machine.error(), QStateMachine::NoInitialStateError);

    // The machine can be corrected and run again, and a run that is stopped then reports no error of this one
    compoundState->setInitialState(childState);
    QVERIFY(startAndWaitForEntered(&machine, childState));
    QCOMPARE(machine.error(), QStateMachine::NoError);
    machine.stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stoppedSpy, 2, TestTimeout::shortMs());
}

void QGCStateMachineTest::_testMachineAsStateOfAnotherMachine_data()
{
    QTest::addColumn<bool>("leftBeforeItFinishes");

    QTest::newRow("the nested machine finishes") << false;
    QTest::newRow("the outer machine leaves the nested one first") << true;
}

void QGCStateMachineTest::_testMachineAsStateOfAnotherMachine()
{
    QFETCH(bool, leftBeforeItFinishes);

    QGCStateMachine outer(QStringLiteral("OuterMachine"), nullptr);
    auto* nested = new QGCStateMachine(QStringLiteral("NestedMachine"), nullptr, &outer);
    auto* nestedWait = new QGCState(QStringLiteral("NestedWait"), nested);
    nested->addEventTransition(nestedWait, QStringLiteral("finish"), nested->addFinalState());
    nested->setInitialState(nestedWait);

    auto* finalState = outer.addFinalState();
    outer.addTransition(nested, &QState::finished, finalState);
    outer.addEventTransition(nested, QStringLiteral("leave"), finalState);
    outer.setInitialState(nested);

    QSignalSpy nestedInProgressSpy(nested, &QGCStateMachine::inProgressChanged);
    QSignalSpy nestedFinishedSpy(nested, &QStateMachine::finished);
    QSignalSpy nestedStoppedSpy(nested, &QStateMachine::stopped);
    QSignalSpy outerFinishedSpy(&outer, &QStateMachine::finished);

    // The outer machine starts the nested one when it enters it
    QVERIFY(startAndWaitForEntered(&outer, nestedWait));
    QVERIFY(outer.configuration().contains(nested));
    QVERIFY(nested->inProgress());
    QCOMPARE(nestedInProgressSpy.count(), 1);

    if (leftBeforeItFinishes) {
        // ... and stops it when it leaves it
        outer.postEvent(QStringLiteral("leave"));
        QVERIFY(spyTriggered(nestedStoppedSpy));
        QCOMPARE(nestedFinishedSpy.count(), 0);
    } else {
        // ... and takes its finished() like that of any other state
        nested->postEvent(QStringLiteral("finish"));
        QVERIFY(spyTriggered(nestedFinishedSpy));
        QCOMPARE(nestedStoppedSpy.count(), 0);
    }

    QVERIFY(spyTriggered(outerFinishedSpy));
    QVERIFY(!nested->inProgress());
    QCOMPARE(nestedInProgressSpy.count(), 2);
}

// -----------------------------------------------------------------------------
// Transition Builder Tests
// -----------------------------------------------------------------------------

void QGCStateMachineTest::_testTransitionBuildersTakeSignalOfDerivedState()
{
    // advance() is declared by a class derived from QState: the builders take it from a state of that class
    QGCStateMachine machine(QStringLiteral("DerivedSignalBuilderTest"), nullptr);
    auto* first = new FunctionState(QStringLiteral("First"), &machine, []() {});
    auto* second = new FunctionState(QStringLiteral("Second"), &machine, []() {});
    auto* third = new FunctionState(QStringLiteral("Third"), &machine, []() {});
    auto* finalState = machine.addFinalState();
    bool guardCalled = false;

    QSignalTransition* const plain = machine.addTransition(first, &QGCState::advance, second);
    GuardedTransition* const guarded = machine.addGuardedTransition(second, &QGCState::advance, third, [&]() {
        guardCalled = true;
        return true;
    });
    // By name, as before
    QSignalTransition* const named = machine.addTransition(third, SIGNAL(advance()), finalState);
    machine.setInitialState(first);

    QCOMPARE(plain->sourceState(), first);
    QCOMPARE(plain->senderObject(), first);
    QCOMPARE(plain->targetState(), second);
    QCOMPARE(guarded->sourceState(), second);
    QCOMPARE(guarded->senderObject(), second);
    QCOMPARE(guarded->targetState(), third);
    QCOMPARE(named->sourceState(), third);
    QCOMPARE(named->senderObject(), third);
    QCOMPARE(named->targetState(), finalState);

    QSignalSpy secondEnteredSpy(second, &QState::entered);
    QSignalSpy thirdEnteredSpy(third, &QState::entered);
    QVERIFY(startAndWaitForFinished(&machine));
    QCOMPARE(secondEnteredSpy.count(), 1);
    QCOMPARE(thirdEnteredSpy.count(), 1);
    QVERIFY(guardCalled);

    // The machine is a state as well: a transition of the machine itself, on a signal that its class declares
    QSignalTransition* const fromMachine =
        machine.addTransition(&machine, &QGCStateMachine::inProgressChanged, finalState);
    QCOMPARE(fromMachine->sourceState(), &machine);
    QCOMPARE(fromMachine->senderObject(), &machine);

    // So does the retry builder
    RetryTransition* const retryFromMachine =
        machine.addRetryTransition(&machine, &QGCStateMachine::inProgressChanged, finalState, []() {});
    QCOMPARE(retryFromMachine->sourceState(), &machine);
    QCOMPARE(retryFromMachine->senderObject(), &machine);
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
    QSignalSpy state2EnteredSpy(state2, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    QVERIFY(startAndWaitForEntered(&machine, state1));

    machine.setSubProgress(0.5f);
    machine.postEvent(QStringLiteral("toState2"));
    QVERIFY(spyTriggered(state2EnteredSpy, TestTimeout::shortMs()));

    machine.setSubProgress(0.5f);
    machine.setSubProgress(0.4f);  // lower value should not emit
    machine.setSubProgress(1.0f);

    machine.postEvent(QStringLiteral("finish"));
    QVERIFY(spyTriggered(finishedSpy, TestTimeout::shortMs()));

    QCOMPARE(progressSpy.count(), 4);
    QVERIFY(qAbs(_spyFloatArg(progressSpy, 0) - 0.125f) < 0.001f);
    QVERIFY(qAbs(_spyFloatArg(progressSpy, 1) - 0.25f) < 0.001f);
    QVERIFY(qAbs(_spyFloatArg(progressSpy, 2) - 0.625f) < 0.001f);
    QVERIFY(qAbs(_spyFloatArg(progressSpy, 3) - 1.0f) < 0.001f);
    QVERIFY(qAbs(machine.progress() - 1.0f) < 0.001f);
}

void QGCStateMachineTest::_testRepeatedRestartRequests_data()
{
    QTest::addColumn<bool>("firstFromStoppedListener");

    QTest::newRow("restart() twice") << false;
    // The listener is called before the restart that is pending, and finds the machine stopped
    QTest::newRow("restart(), and again from a listener of stopped") << true;
}

void QGCStateMachineTest::_testRepeatedRestartRequests()
{
    QFETCH(bool, firstFromStoppedListener);

    QGCStateMachine machine(QStringLiteral("RepeatedRestartTest"), nullptr);
    // A state that waits, so that both requests meet a machine in progress
    auto* first = new QGCState(QStringLiteral("First"), &machine);
    machine.setInitialState(first);

    QSignalSpy firstEnteredSpy(first, &QState::entered);
    QSignalSpy startedSpy(&machine, &QStateMachine::started);
    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);

    QVERIFY(startAndWaitForEntered(&machine, first));

    // Both before the machine has stopped
    if (firstFromStoppedListener) {
        (void) connect(
            &machine, &QStateMachine::stopped, this, [&machine]() { machine.restart(); }, Qt::SingleShotConnection);
    } else {
        machine.restart();
    }
    machine.restart();

    // The machine restarts once
    QTRY_COMPARE_WITH_TIMEOUT(firstEnteredSpy.count(), 2, TestTimeout::shortMs());
    QCOMPARE(stoppedSpy.count(), 1);
    QCOMPARE(startedSpy.count(), 2);
    QVERIFY(machine.inProgress());

    // A later request restarts it again
    machine.restart();
    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 3, TestTimeout::shortMs());
    QCOMPARE(stoppedSpy.count(), 2);
}

void QGCStateMachineTest::_testRestartResetsProgress_data()
{
    QTest::addColumn<bool>("progressFirst");
    QTest::addColumn<bool>("stopFirst");

    QTest::newRow("restart() in progress") << true << false;
    QTest::newRow("restart() after the machine has stopped") << true << true;
    QTest::newRow("restart() before any progress") << false << false;
}

void QGCStateMachineTest::_testRestartResetsProgress()
{
    QFETCH(bool, progressFirst);
    QFETCH(bool, stopFirst);

    QGCStateMachine machine(QStringLiteral("RestartProgressTest"), nullptr);
    auto* first = new QGCState(QStringLiteral("First"), &machine);
    auto* second = new QGCState(QStringLiteral("Second"), &machine);
    machine.addEventTransition(first, QStringLiteral("next"), second);
    machine.setProgressWeights({{first, 1}, {second, 1}});
    machine.setInitialState(first);

    QSignalSpy firstEnteredSpy(first, &QState::entered);
    QSignalSpy secondEnteredSpy(second, &QState::entered);
    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);

    QVERIFY(startAndWaitForEntered(&machine, first));
    if (progressFirst) {
        machine.postEvent(QStringLiteral("next"));
        QVERIFY(spyTriggered(secondEnteredSpy));
    }
    QCOMPARE(machine.progress(), progressFirst ? 0.5f : 0.0f);

    if (stopFirst) {
        machine.stop();
        QVERIFY(spyTriggered(stoppedSpy));
    }

    QSignalSpy progressSpy(&machine, &QGCStateMachine::progressUpdate);
    machine.restart();
    QTRY_COMPARE_WITH_TIMEOUT(firstEnteredSpy.count(), 2, TestTimeout::shortMs());

    // The machine is in its first state again. It has said so if its progress had to go back for it
    QCOMPARE(machine.progress(), 0.0f);
    QCOMPARE(progressSpy.count(), progressFirst ? 1 : 0);
    if (progressFirst) {
        QCOMPARE(progressSpy.takeFirst().at(0).toFloat(), 0.0f);
    }

    // It reports its progress from there
    machine.setSubProgress(0.5f);
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(machine.progress(), 0.25f);
}

void QGCStateMachineTest::_testRestartWhenFinishedResetsProgress()
{
    QGCStateMachine machine(QStringLiteral("RestartWhenFinishedTest"), nullptr);
    auto* first = new QGCState(QStringLiteral("First"), &machine);
    QFinalState* finalState = addFinalState(&machine);
    machine.addEventTransition(first, QStringLiteral("done"), finalState);
    machine.setProgressWeights({{first, 1}});
    machine.setInitialState(first);

    // A machine that finishes says that it is no longer in progress before it reports the progress of the run
    // that has ended, so this listener restarts it in between
    bool restarted = false;
    connect(&machine, &QGCStateMachine::inProgressChanged, &machine, [&machine, &restarted]() {
        if (!machine.inProgress() && !restarted) {
            restarted = true;
            machine.restart();
        }
    });

    QSignalSpy firstEnteredSpy(first, &QState::entered);
    QVERIFY(startAndWaitForEntered(&machine, first));
    machine.postEvent(QStringLiteral("done"));
    QTRY_COMPARE_WITH_TIMEOUT(firstEnteredSpy.count(), 2, TestTimeout::shortMs());
    QVERIFY(restarted);

    QCOMPARE(machine.progress(), 0.0f);
    QSignalSpy progressSpy(&machine, &QGCStateMachine::progressUpdate);
    machine.setSubProgress(0.5f);
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(machine.progress(), 0.5f);
}

void QGCStateMachineTest::_testProgressWeightsReplaced()
{
    QGCStateMachine machine(QStringLiteral("ProgressWeightsReplacedTest"), nullptr);
    auto* state1 = new QGCState(QStringLiteral("State1"), &machine);
    auto* state2 = new QGCState(QStringLiteral("State2"), &machine);
    machine.addEventTransition(state1, QStringLiteral("next"), state2);
    machine.setProgressWeights({{state1, 1}, {state2, 1}});
    machine.setInitialState(state1);

    QSignalSpy state2EnteredSpy(state2, &QState::entered);
    QVERIFY(startAndWaitForEntered(&machine, state1));
    machine.postEvent(QStringLiteral("next"));
    QVERIFY(spyTriggered(state2EnteredSpy));
    QCOMPARE(machine.progress(), 0.5f);

    // The progress of a new list starts at 0, and stays there until one of its states is entered. An entry
    // without a state or without a weight is left out.
    QSignalSpy progressSpy(&machine, &QGCStateMachine::progressUpdate);
    const QRegularExpression ignored(QStringLiteral("progress weight ignored"));
    expectLogMessage("Utilities.QGCStateMachine", QtWarningMsg, ignored);
    expectLogMessage("Utilities.QGCStateMachine", QtWarningMsg, ignored);
    machine.setProgressWeights({{state1, 2}, {nullptr, 1}, {state2, 0}});
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(_spyFloatArg(progressSpy, 0), 0.0f);
    QCOMPARE(machine.progress(), 0.0f);
    machine.setSubProgress(1.0f);
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(machine.progress(), 0.0f);
}

UT_REGISTER_TEST(QGCStateMachineTest, TestLabel::Unit, TestLabel::Utilities)
