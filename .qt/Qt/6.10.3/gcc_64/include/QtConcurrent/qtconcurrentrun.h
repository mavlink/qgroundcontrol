// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTCONCURRENT_RUN_H
#define QTCONCURRENT_RUN_H

#if 0
#pragma qt_class(QtConcurrentRun)
#endif

#include <QtConcurrent/qtconcurrentcompilertest.h>

#if !defined(QT_NO_CONCURRENT) || defined(Q_QDOC)

#include <QtConcurrent/qtconcurrentrunbase.h>
#include <QtConcurrent/qtconcurrentstoredfunctioncall.h>

QT_BEGIN_NAMESPACE

#ifdef Q_QDOC

typedef int Function;

namespace QtConcurrent {

    template <typename T>
    QFuture<T> run(Function function, ...);

    template <typename T>
    QFuture<T> run(QThreadPool *pool, Function function, ...);

} // namespace QtConcurrent

#else

namespace QtConcurrent {

#define QTCONCURRENT_RUN_NODISCARD \
    Q_NODISCARD_X("Use QThreadPool::start(Callable&&) if you don't need the returned QFuture")

template <class Function, class ...Args>
QTCONCURRENT_RUN_NODISCARD
auto run(QThreadPool *pool, Function &&f, Args &&...args)
{
    DecayedTuple<Function, Args...> tuple { std::forward<Function>(f),
                                            std::forward<Args>(args)... };
    return TaskResolver<std::decay_t<Function>, std::decay_t<Args>...>::run(
                std::move(tuple), TaskStartParameters { pool });
}

template <class Function, class ...Args>
QTCONCURRENT_RUN_NODISCARD
auto run(QThreadPool *pool, std::reference_wrapper<const Function> &&functionWrapper,
         Args &&...args)
{
    return run(pool, std::forward<const Function>(functionWrapper.get()),
               std::forward<Args>(args)...);
}

template <class Function, class ...Args>
QTCONCURRENT_RUN_NODISCARD
auto run(Function &&f, Args &&...args)
{
    return run(QThreadPool::globalInstance(), std::forward<Function>(f),
               std::forward<Args>(args)...);
}

// overload with a Promise Type hint, takes thread pool
template <class PromiseType, class Function, class ...Args>
QTCONCURRENT_RUN_NODISCARD
auto run(QThreadPool *pool, Function &&f, Args &&...args)
{
    return (new StoredFunctionCallWithPromise<Function, PromiseType, Args...>(
                std::forward<Function>(f), std::forward<Args>(args)...))->start(pool);
}

// overload with a Promise Type hint, uses global thread pool
template <class PromiseType, class Function, class ...Args>
QTCONCURRENT_RUN_NODISCARD
auto run(Function &&f, Args &&...args)
{
    return run<PromiseType>(QThreadPool::globalInstance(), std::forward<Function>(f),
                            std::forward<Args>(args)...);
}

#undef QTCONCURRENT_RUN_NODISCARD

} //namespace QtConcurrent

#endif // Q_QDOC

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
