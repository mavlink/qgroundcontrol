#pragma once

#include "StateMachineTest.h"

#include "MultiSignalSpy.h"
#include "QGCStateMachine.h"

#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

/// Start the machine and wait until @a state is entered.
/// @return true if the state was entered within the timeout
inline bool startAndWaitForEntered(QStateMachine* machine, QAbstractState* state,
                                   int timeoutMs = TestTimeout::shortMs())
{
    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    if (auto* qgcMachine = qobject_cast<QGCStateMachine*>(machine)) {
        qgcMachine->start();
    } else {
        machine->start();
    }
    return (enteredSpy.count() > 0) || enteredSpy.wait(timeoutMs);
}

/// Makes @p state the initial state of @p machine and adds a final state that it goes to on @p signal
template <typename State, typename Signal>
void finishOn(QStateMachine& machine, State* state, Signal signal)
{
    state->addTransition(state, signal, new QFinalState(&machine));
    machine.setInitialState(state);
}

/// A WaitStateBase that a test completes or fails itself, and that tells what became of its wait signal and how
/// often its wait ended
class TestWaitState : public WaitStateBase
{
public:
    TestWaitState(const QString& name, QState* parent, int timeoutMsecs = 0)
        : WaitStateBase(name, parent, timeoutMsecs)
    {}

    void triggerComplete() { waitComplete(); }

    void triggerFailed() { waitFailed(); }

    bool connectCalled = false;
    bool disconnectCalled = false;
    bool waitSignalConnected = false;
    int waitExitedCalls = 0;

protected:
    void connectWaitSignal() override
    {
        connectCalled = true;
        waitSignalConnected = true;
    }

    void disconnectWaitSignal() override
    {
        disconnectCalled = true;
        waitSignalConnected = false;
    }

    void onWaitExited() override { waitExitedCalls++; }
};

/// A QGCAbstractState whose signals a test emits
class TestAbstractState : public QGCAbstractState
{
public:
    TestAbstractState(const QString& name, QState* parent)
        : QGCAbstractState(name, parent)
    {}

    void emitAdvance() { emit advance(); }

    void emitError() { emit error(); }
};
