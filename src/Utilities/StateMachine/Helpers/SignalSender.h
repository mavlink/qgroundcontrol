#pragma once

/// \brief The class of the object that emits a signal, from the type of the pointer to the signal
///
/// A template that takes a sender and a signal declares the sender with it, so that only the signal is deduced and
/// the sender converts to the signal's class, as it does for QObject::connect():
/// @code
/// template<typename Func>
/// void waitFor(const SignalSender<Func>* sender, Func signal);
///
/// waitFor(functionState, &QGCState::advance);
/// @endcode
/// Only a pointer to a member function has a sender: one that is plain, const, noexcept or const noexcept, which
/// are the ones QObject::connect() takes. For anything else (a functor, a function, a signal name) there is no
/// `type`, which removes the template from overload resolution.
template <typename Func>
struct SignalSenderType
{
};

template <typename Obj, typename Ret, typename... Args>
struct SignalSenderType<Ret (Obj::*)(Args...)>
{
    using type = Obj;
};

template <typename Obj, typename Ret, typename... Args>
struct SignalSenderType<Ret (Obj::*)(Args...) const>
{
    using type = Obj;
};

template <typename Obj, typename Ret, typename... Args>
struct SignalSenderType<Ret (Obj::*)(Args...) noexcept>
{
    using type = Obj;
};

template <typename Obj, typename Ret, typename... Args>
struct SignalSenderType<Ret (Obj::*)(Args...) const noexcept>
{
    using type = Obj;
};

template <typename Func>
using SignalSender = typename SignalSenderType<Func>::type;
