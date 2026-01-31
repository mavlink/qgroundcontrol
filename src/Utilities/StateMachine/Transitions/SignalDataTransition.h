#pragma once

#include "QGCSignalTransition.h"
#include "QGCState.h"

#include <QtStateMachine/QStateMachine>
#include <functional>

/// Transition that provides access to signal arguments in guard and action
/// Use this when you need to inspect signal parameters to decide whether to transition.
template<typename... Args>
class SignalDataTransition : public QGCSignalTransition
{
public:
    SignalDataTransition(const SignalDataTransition&) = delete;
    SignalDataTransition& operator=(const SignalDataTransition&) = delete;

    using Guard = std::function<bool(Args...)>;
    using Action = std::function<void(Args...)>;

    /// Create a signal data transition
    /// @param sender The object that emits the signal
    /// @param signal The signal to listen for
    /// @param target Target state for the transition
    /// @param guard Optional predicate receiving signal args, must return true to transition
    /// @param action Optional action receiving signal args, called during transition
    template<typename Sender, typename Func>
    SignalDataTransition(const Sender* sender, Func signal, QAbstractState* target,
                         Guard guard = nullptr, Action action = nullptr)
        : QGCSignalTransition(sender, signal)
        , _guard(std::move(guard))
        , _action(std::move(action))
    {
        setTargetState(target);
    }

    /// Get the arguments from the last matched signal
    std::tuple<Args...> signalArgs() const { return _lastArgs; }

protected:
    bool eventTest(QEvent* event) override
    {
        if (!QGCSignalTransition::eventTest(event)) {
            return false;
        }

        // Extract signal arguments from SignalEvent
        auto* signalEvent = static_cast<QStateMachine::SignalEvent*>(event);
        const QList<QVariant>& arguments = signalEvent->arguments();

        // Convert QVariant list to tuple
        _lastArgs = extractArgs(arguments, std::index_sequence_for<Args...>{});

        if (_guard) {
            bool allowed = std::apply(_guard, _lastArgs);
            if (!allowed) {
                qCDebug(QGCStateMachineLog) << "SignalDataTransition blocked by guard";
                return false;
            }
        }

        return true;
    }

    void onTransition(QEvent* event) override
    {
        Q_UNUSED(event);

        if (_action) {
            std::apply(_action, _lastArgs);
        }
    }

private:
    template<std::size_t... Is>
    std::tuple<Args...> extractArgs(const QList<QVariant>& arguments, std::index_sequence<Is...>)
    {
        return std::make_tuple(
            (Is < static_cast<std::size_t>(arguments.size())
                ? arguments[Is].value<std::tuple_element_t<Is, std::tuple<Args...>>>()
                : std::tuple_element_t<Is, std::tuple<Args...>>{})...
        );
    }

    Guard _guard;
    Action _action;
    mutable std::tuple<Args...> _lastArgs;
};

/// Helper to create SignalDataTransition with type deduction
/// Usage: makeSignalDataTransition<bool>(vehicle, &Vehicle::armedChanged, target, guard)
template<typename... Args, typename Sender, typename Func>
SignalDataTransition<Args...>* makeSignalDataTransition(
    const Sender* sender, Func signal, QAbstractState* target,
    typename SignalDataTransition<Args...>::Guard guard = nullptr,
    typename SignalDataTransition<Args...>::Action action = nullptr)
{
    return new SignalDataTransition<Args...>(sender, signal, target, std::move(guard), std::move(action));
}
