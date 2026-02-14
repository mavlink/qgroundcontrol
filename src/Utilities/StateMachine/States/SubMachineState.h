#pragma once

#include "QGCState.h"

#include <functional>

class QGCStateMachine;

/// State that invokes a child state machine
/// When entered, creates and starts a child state machine.
/// When the child machine finishes, emits advance() or error().
class SubMachineState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(SubMachineState)

public:
    /// Factory function type for creating child state machines
    /// @param parent The SubMachineState that will own the child machine
    /// @return The configured child state machine (do not start it)
    using MachineFactory = std::function<QGCStateMachine*(SubMachineState* parent)>;

    /// Create a sub-machine state
    /// @param stateName Name for debugging
    /// @param parent Parent state or machine
    /// @param factory Function that creates the child state machine
    SubMachineState(const QString& stateName, QState* parent, MachineFactory factory);

    /// Get the currently running child machine (nullptr if not running)
    QGCStateMachine* childMachine() const { return _childMachine; }

signals:
    /// Emitted when the child machine encounters an error
    void childError();

protected:
    void onEntry(QEvent* event) override;
    void onExit(QEvent* event) override;

private slots:
    void _onChildFinished();
    void _onChildError();

private:
    MachineFactory _factory;
    QGCStateMachine* _childMachine = nullptr;
};
