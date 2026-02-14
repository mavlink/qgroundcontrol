#pragma once

#include "QGCState.h"

#include <functional>

/// A state that evaluates a predicate on entry and either executes or skips
/// Useful for conditional logic like "skip if high latency link"
class ConditionalState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(ConditionalState)

public:
    using Predicate = std::function<bool()>;
    using Action = std::function<void()>;

    /// @param stateName Name for this state (for logging)
    /// @param parent Parent state
    /// @param predicate If true, execute action and emit advance(); if false, emit skipped()
    /// @param action Optional action to execute when predicate is true
    ConditionalState(const QString& stateName, QState* parent, Predicate predicate, Action action = Action());

    /// Set the predicate
    void setPredicate(Predicate predicate) { _predicate = std::move(predicate); }

    /// Set the action
    void setAction(Action action) { _action = std::move(action); }

signals:
    /// Emitted when the predicate returns false and the state is skipped
    void skipped();

private slots:
    void _onEntered();

private:
    Predicate _predicate;
    Action _action;
};
