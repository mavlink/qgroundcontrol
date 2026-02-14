#pragma once

#include "QGCState.h"

#include <functional>

/// A state that executes an action with automatic rollback on failure.
///
/// If the forward action fails, the rollback action is executed before
/// emitting the error() signal. This provides transaction-like semantics.
///
/// Example usage:
/// @code
/// auto* state = new RollbackState("UpdateFirmware", &machine,
///     []() { return flashFirmware(); },      // Forward action
///     []() { restorePreviousFirmware(); }    // Rollback action (void)
/// );
///
/// state->addTransition(state, &RollbackState::advance, successState);
/// state->addTransition(state, &RollbackState::error, errorState);
/// @endcode
class RollbackState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(RollbackState)

public:
    /// Forward action - returns true on success
    using ForwardAction = std::function<bool()>;

    /// Rollback action - executed on forward failure
    using RollbackAction = std::function<void()>;

    /// @param stateName Name for logging
    /// @param parent Parent state
    /// @param forwardAction Action to execute (returns true on success)
    /// @param rollbackAction Action to execute on failure (cleanup/restore)
    RollbackState(const QString& stateName, QState* parent,
                  ForwardAction forwardAction, RollbackAction rollbackAction);

    /// Check if rollback was executed
    bool wasRolledBack() const { return _wasRolledBack; }

    /// Check if rollback succeeded (only valid if wasRolledBack() is true)
    bool rollbackSucceeded() const { return _rollbackSucceeded; }

signals:
    /// Emitted when forward action succeeds
    void forwardSucceeded();

    /// Emitted when forward action fails (before rollback)
    void forwardFailed();

    /// Emitted when rollback starts
    void rollingBack();

    /// Emitted when rollback completes
    void rollbackComplete();

protected:
    void onEnter() override;

private:
    ForwardAction _forwardAction;
    RollbackAction _rollbackAction;
    bool _wasRolledBack = false;
    bool _rollbackSucceeded = false;
};
