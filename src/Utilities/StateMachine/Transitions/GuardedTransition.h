#pragma once

#include "QGCSignalTransition.h"

#include <functional>

/// A transition that only fires if a guard predicate returns true
/// Useful for conditional transitions based on runtime state
class GuardedTransition : public QGCSignalTransition
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
    template<typename Func>
    GuardedTransition(const typename QtPrivate::FunctionPointer<Func>::Object* sender,
                      Func signalFn,
                      QAbstractState* target,
                      Guard guard)
        : QGCSignalTransition(sender, signalFn)
        , _guard(std::move(guard))
    {
        setTargetState(target);
    }

    /// Create a guarded transition without target (set later)
    /// @param sender Object that emits the signal
    /// @param signal Signal that triggers the transition
    /// @param guard Predicate that must return true for transition to fire
    template<typename Func>
    GuardedTransition(const typename QtPrivate::FunctionPointer<Func>::Object* sender,
                      Func signalFn,
                      Guard guard)
        : QGCSignalTransition(sender, signalFn)
        , _guard(std::move(guard))
    {
    }

    /// Set the guard predicate
    void setGuard(Guard guard) { _guard = std::move(guard); }

    /// Get the current guard predicate
    Guard guard() const { return _guard; }

protected:
    /// Override to check guard before allowing transition
    bool eventTest(QEvent* event) override;

private:
    Guard _guard;
};
