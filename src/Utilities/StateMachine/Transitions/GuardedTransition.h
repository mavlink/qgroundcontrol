#pragma once

#include <functional>

#include <QtStateMachine/QSignalTransition>

#include "SignalSender.h"

/// \brief A transition that only fires if a guard predicate returns true
///
/// Useful for conditional transitions based on runtime state
///
class GuardedTransition : public QSignalTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(GuardedTransition)

public:
    using Guard = std::function<bool()>;

    /// Create a guarded transition
    /// @param sender Object that emits the signal
    /// @param signal Signal that triggers the transition
    /// @param target Target state
    /// @param guard Predicate that must return true for transition to fire
    template <typename Func>
    GuardedTransition(const SignalSender<Func>* sender, Func signalFn, QAbstractState* target, Guard guard)
        : QSignalTransition(sender, signalFn)
        , _guard(std::move(guard))
    {
        setTargetState(target);
    }

    /// Create a guarded transition without target (set later)
    /// @param sender Object that emits the signal
    /// @param signal Signal that triggers the transition
    /// @param guard Predicate that must return true for transition to fire
    template <typename Func>
    GuardedTransition(const SignalSender<Func>* sender, Func signalFn, Guard guard)
        : QSignalTransition(sender, signalFn)
        , _guard(std::move(guard))
    {
    }

protected:
    /// Override to check guard before allowing transition
    bool eventTest(QEvent* event) override;

private:
    Guard _guard;
};
