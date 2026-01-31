#pragma once

#include "QGCSignalTransition.h"

#include <functional>

/// Transition that fires without exiting/re-entering the current state
/// Useful for handling repeated signals (like heartbeats) without state churn.
/// The action is executed but no state exit/entry occurs.
class InternalTransition : public QGCSignalTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(InternalTransition)

public:
    using Action = std::function<void()>;

    /// Create an internal transition with an action
    /// @param sender The object that emits the signal
    /// @param signal The signal to listen for
    /// @param action Function to execute when transition fires (optional)
    template<typename Func>
    InternalTransition(const typename QtPrivate::FunctionPointer<Func>::Object* sender,
                       Func signal, Action action = nullptr)
        : QGCSignalTransition(sender, signal)
        , _action(std::move(action))
    {
        setTransitionType(QAbstractTransition::InternalTransition);
        // Internal transitions target the source state (stay in same state)
    }

protected:
    void onTransition(QEvent* event) override;

private:
    Action _action;
};
