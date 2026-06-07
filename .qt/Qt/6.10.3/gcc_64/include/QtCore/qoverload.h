// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOVERLOAD_H
#define QOVERLOAD_H

#include <QtCore/qtconfigmacros.h>

#if 0
#pragma qt_class(QOverload)
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_QDOC
// Just for documentation generation
template<typename T>
auto qOverload(T functionPointer);
template<typename T>
auto qConstOverload(T memberFunctionPointer);
template<typename T>
auto qNonConstOverload(T memberFunctionPointer);
#else
template <typename... Args>
struct QNonConstOverload
{
    template <typename R, typename T>
    constexpr auto operator()(R (T::*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename T>
    static constexpr auto of(R (T::*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
struct QConstOverload
{
    template <typename R, typename T>
    constexpr auto operator()(R (T::*ptr)(Args...) const) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename T>
    static constexpr auto of(R (T::*ptr)(Args...) const) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
{
    using QConstOverload<Args...>::of;
    using QConstOverload<Args...>::operator();
    using QNonConstOverload<Args...>::of;
    using QNonConstOverload<Args...>::operator();

    template <typename R>
    constexpr auto operator()(R (*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R>
    static constexpr auto of(R (*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args> constexpr inline QOverload<Args...> qOverload = {};
template <typename... Args> constexpr inline QConstOverload<Args...> qConstOverload = {};
template <typename... Args> constexpr inline QNonConstOverload<Args...> qNonConstOverload = {};

#endif // Q_QDOC

#define QT_VA_ARGS_CHOOSE(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define QT_VA_ARGS_EXPAND(...) __VA_ARGS__ // Needed for MSVC
#define QT_VA_ARGS_COUNT(...) QT_VA_ARGS_EXPAND(QT_VA_ARGS_CHOOSE(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define QT_OVERLOADED_MACRO_EXPAND(MACRO, ARGC) MACRO##_##ARGC
#define QT_OVERLOADED_MACRO_IMP(MACRO, ARGC) QT_OVERLOADED_MACRO_EXPAND(MACRO, ARGC)
#define QT_OVERLOADED_MACRO(MACRO, ...) QT_VA_ARGS_EXPAND(QT_OVERLOADED_MACRO_IMP(MACRO, QT_VA_ARGS_COUNT(__VA_ARGS__))(__VA_ARGS__))

QT_END_NAMESPACE

#endif /* QOVERLOAD_H */
