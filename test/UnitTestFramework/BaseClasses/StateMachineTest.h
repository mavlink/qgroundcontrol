#pragma once

#include <QtStateMachine/QFinalState>
#include <QtStateMachine/QStateMachine>

#include "UnitTest.h"

class QSignalSpy;

/// Test fixture for QStateMachine-based state and transition tests.
///
/// Eliminates the repeated boilerplate of creating a machine, adding a
/// QFinalState, wiring the advance signal, starting, and waiting.
///
/// Example (single-state run):
/// @code
/// class MyStateTest : public StateMachineTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testMyState() {
///         QStateMachine machine;
///         auto* state = new MyState("name", &machine);
///         QVERIFY(runStateToCompletion(state, &machine));
///     }
/// };
/// @endcode
///
/// Example (custom wiring):
/// @code
/// void _testChain() {
///     QStateMachine machine;
///     auto* s1 = new FunctionState("s1", &machine, []{ ... });
///     auto* s2 = new FunctionState("s2", &machine, []{ ... });
///     auto* final = addFinalState(&machine);
///     s1->addTransition(s1, &QGCState::advance, s2);
///     s2->addTransition(s2, &QGCState::advance, final);
///     machine.setInitialState(s1);
///     QVERIFY(startAndWaitForFinished(&machine));
/// }
/// @endcode
class StateMachineTest : public UnitTest
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(StateMachineTest)

public:
    explicit StateMachineTest(QObject* parent = nullptr) : UnitTest(parent)
    {
    }

protected:
    /// Run a single state to completion using the default advance() signal.
    /// Creates a QFinalState, wires state's advance→final, sets initial state, starts, waits.
    /// @param state The state to run (must already be parented to @a machine)
    /// @param machine The state machine owning @a state
    /// @param timeoutMs Maximum wait for QStateMachine::finished
    /// @return true if machine finished within timeout
    template <typename State>
    bool runStateToCompletion(State* state, QStateMachine* machine, int timeoutMs = TestTimeout::shortMs())
    {
        auto* final = new QFinalState(machine);
        state->addTransition(state, &State::advance, final);
        machine->setInitialState(state);
        return startAndWaitForFinished(machine, timeoutMs);
    }

    /// Add a QFinalState to the machine and return it.
    static QFinalState* addFinalState(QStateMachine* machine)
    {
        return new QFinalState(machine);
    }

    /// Start the machine and wait for QStateMachine::finished.
    /// @return true if the machine reached a final state within timeout
    static bool startAndWaitForFinished(QStateMachine* machine, int timeoutMs = TestTimeout::shortMs());

    /// Check if spy was already triggered or wait for it.
    /// Replaces the `_spyTriggered` free function duplicated in 5+ state machine test files.
    static bool spyTriggered(QSignalSpy& spy, int timeoutMs = TestTimeout::shortMs());
};
