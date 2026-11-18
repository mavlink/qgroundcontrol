#include <functional>
#include <type_traits>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtStateMachine/QState>

#include "AsyncFunctionState.h"
#include "FunctionState.h"
#include "GuardedTransition.h"
#include "QGCState.h"
#include "QGCStateMachine.h"
#include "RetryTransition.h"
#include "SignalSender.h"
#include "SkippableAsyncState.h"

// Compile-time checks only: the senders and signals that the templates taking a signal accept, and those they
// reject.

namespace SignalSenderShapes {

using Signal = decltype(&QObject::objectNameChanged);
using DerivedClassSignal = decltype(&QTimer::timeout);
using ConstMember = decltype(&QObject::objectName);
using NoexceptMember = void (QObject::*)() noexcept;
using ConstNoexceptMember = void (QObject::*)() const noexcept;
using VolatileMember = void (QObject::*)() volatile;
using RefQualifiedMember = void (QObject::*)() &;
using DataMember = int QObject::*;
using FreeFunction = void (*)();
using Functor = std::function<void()>;

template <typename Sender, typename Func>
struct Guarded : std::is_constructible<GuardedTransition, Sender, Func, GuardedTransition::Guard>
{
};

template <typename Sender, typename Func>
struct GuardedWithTarget
    : std::is_constructible<GuardedTransition, Sender, Func, QAbstractState*, GuardedTransition::Guard>
{
};

template <typename Sender, typename Func>
struct Retry : std::is_constructible<RetryTransition, Sender, Func, QAbstractState*, RetryTransition::RetryAction, int>
{
};

template <typename State, typename Sender, typename Func>
concept CompletesOnSignal =
    requires(State& state, Sender sender, Func signal) { state.connectToCompletion(sender, signal); };

template <typename State, typename Sender, typename Func>
concept CompletesOnSignalWithSlot = requires(State& state, Sender sender, Func signal, Functor slot) {
    state.connectToCompletion(sender, signal, slot);
};

template <typename State>
struct ConnectToCompletion
{
    template <typename Sender, typename Func>
    struct OnSignal : std::bool_constant<CompletesOnSignal<State, Sender, Func>>
    {
    };

    template <typename Sender, typename Func>
    struct OnSignalWithSlot : std::bool_constant<CompletesOnSignalWithSlot<State, Sender, Func>>
    {
    };
};

/// What every template that takes a sender and a signal accepts and rejects
template <template <typename, typename> typename Accepts>
struct Common
{
    static_assert(Accepts<QObject*, Signal>::value, "must accept a sender and a signal of its class");
    static_assert(Accepts<QTimer*, Signal>::value, "must accept a sender derived from the signal's class");
    static_assert(Accepts<QPointer<QTimer>, Signal>::value, "must accept a sender that converts to a pointer");
    static_assert(Accepts<QObject*, ConstMember>::value, "must accept a const member function");
    static_assert(Accepts<QObject*, NoexceptMember>::value, "must accept a noexcept member function");
    static_assert(Accepts<QObject*, ConstNoexceptMember>::value, "must accept a const noexcept member function");

    static_assert(!Accepts<QObject*, DerivedClassSignal>::value,
                  "must reject a sender that is not of the signal's class");
    static_assert(!Accepts<QObject*, VolatileMember>::value, "must reject a volatile member function");
    static_assert(!Accepts<QObject*, RefQualifiedMember>::value, "must reject a ref-qualified member function");
    static_assert(!Accepts<QObject*, DataMember>::value, "must reject a data member");
    static_assert(!Accepts<QObject*, FreeFunction>::value, "must reject a function");
    static_assert(!Accepts<QObject*, Functor>::value, "must reject a functor");
};

/// Transitions take their sender as a pointer to const
template <template <typename, typename> typename Accepts>
struct Transition : Common<Accepts>
{
    static_assert(Accepts<const QObject*, Signal>::value, "must accept a const sender");
};

/// States take their sender as a pointer to non-const
template <template <typename, typename> typename Accepts>
struct State : Common<Accepts>
{
    static_assert(!Accepts<const QObject*, Signal>::value, "must reject a const sender");
};

template struct Transition<Guarded>;
template struct Transition<GuardedWithTarget>;
template struct Transition<Retry>;
template struct State<ConnectToCompletion<AsyncFunctionState>::OnSignal>;
template struct State<ConnectToCompletion<AsyncFunctionState>::OnSignalWithSlot>;
template struct State<ConnectToCompletion<SkippableAsyncState>::OnSignal>;
template struct State<ConnectToCompletion<SkippableAsyncState>::OnSignalWithSlot>;

// The sender is of the class that declares the signal, whichever class the signal is named through
static_assert(std::is_same_v<SignalSender<decltype(&QTimer::timeout)>, QTimer>);
static_assert(std::is_same_v<SignalSender<decltype(&QTimer::objectNameChanged)>, QObject>);
static_assert(std::is_same_v<SignalSender<ConstMember>, QObject>);
static_assert(std::is_same_v<SignalSender<NoexceptMember>, QObject>);
static_assert(std::is_same_v<SignalSender<ConstNoexceptMember>, QObject>);

// The transition factories of QGCStateMachine that take one state as the source of the transition and as the
// sender of its signal

using StateSignal = decltype(&QState::finished);
using InheritedStateSignal = decltype(&QState::entered);
using DerivedStateSignal = decltype(&QGCState::advance);

template <typename SourceState, typename Func>
concept MachineAddsTransition = requires(QGCStateMachine& machine, SourceState from, Func signal,
                                         QAbstractState* target) { machine.addTransition(from, signal, target); };

template <typename SourceState, typename Func>
concept MachineAddsGuardedTransition =
    requires(QGCStateMachine& machine, SourceState from, Func signal, QAbstractState* target,
             GuardedTransition::Guard guard) { machine.addGuardedTransition(from, signal, target, guard); };

template <typename SourceState, typename Func>
concept MachineAddsRetryTransition =
    requires(QGCStateMachine& machine, SourceState from, Func signal, QAbstractState* target,
             RetryTransition::RetryAction action) { machine.addRetryTransition(from, signal, target, action); };

template <typename SourceState, typename Func>
struct MachineTransition : std::bool_constant<MachineAddsTransition<SourceState, Func>>
{
};

template <typename SourceState, typename Func>
struct MachineGuardedTransition : std::bool_constant<MachineAddsGuardedTransition<SourceState, Func>>
{
};

template <typename SourceState, typename Func>
struct MachineRetryTransition : std::bool_constant<MachineAddsRetryTransition<SourceState, Func>>
{
};

template <template <typename, typename> typename Accepts>
struct SourceStateFactory
{
    static_assert(Accepts<QState*, StateSignal>::value, "must accept a state and a signal of QState");
    static_assert(Accepts<QState*, InheritedStateSignal>::value, "must accept a signal that QState inherits");
    static_assert(Accepts<QState*, Signal>::value, "must accept a signal of QObject");
    static_assert(Accepts<QGCState*, DerivedStateSignal>::value, "must accept a signal of a class derived from QState");
    static_assert(Accepts<FunctionState*, DerivedStateSignal>::value,
                  "must accept a state derived from the signal's class");
    static_assert(Accepts<QGCState*, StateSignal>::value, "must accept a derived state and a signal of QState");
    static_assert(Accepts<QPointer<QGCState>, DerivedStateSignal>::value,
                  "must accept a state that converts to a pointer");

    static_assert(!Accepts<QState*, DerivedStateSignal>::value,
                  "must reject a state that is not of the signal's class");
    static_assert(!Accepts<const QGCState*, DerivedStateSignal>::value, "must reject a const state");
    static_assert(!Accepts<QAbstractState*, InheritedStateSignal>::value,
                  "must reject a state that cannot have transitions");
    static_assert(!Accepts<QTimer*, DerivedClassSignal>::value, "must reject a sender that is not a state");
    static_assert(!Accepts<QState*, DataMember>::value, "must reject a data member");
    static_assert(!Accepts<QState*, FreeFunction>::value, "must reject a function");
    static_assert(!Accepts<QState*, Functor>::value, "must reject a functor");
};

template struct SourceStateFactory<MachineTransition>;
template struct SourceStateFactory<MachineGuardedTransition>;
template struct SourceStateFactory<MachineRetryTransition>;

// A signal name selects the overload that takes the signal as a string
static_assert(MachineAddsTransition<QState*, const char*>);

/// Never called. A requires-expression checks a declaration and not the body behind it, so this compiles the
/// bodies of the factories with a state and a signal of its own class, one derived from QState.
void compileTransitionFactories(QGCStateMachine& machine, FunctionState* state, QAbstractState* target)
{
    machine.addTransition(state, &QGCState::advance, target);
    machine.addGuardedTransition(state, &QGCState::advance, target, [] { return true; });
    machine.addRetryTransition(state, &QGCState::advance, target, [] {});

    // A machine is a state too, and hides QState::addTransition() with its own factory
    machine.addTransition(&machine, &QGCStateMachine::inProgressChanged, target);
    machine.addGuardedTransition(&machine, &QGCStateMachine::inProgressChanged, target, [] { return true; });
    machine.addRetryTransition(&machine, &QGCStateMachine::inProgressChanged, target, [] {});
}

}  // namespace SignalSenderShapes
