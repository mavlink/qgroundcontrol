// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFUTURE_H
#error Do not include qfuture_impl.h directly
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qfunctionaltools_impl.h>
#include <QtCore/qfutureinterface.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/qexception.h>
#include <QtCore/qpromise.h>
#include <QtCore/qvariant.h>

#include <memory>

QT_BEGIN_NAMESPACE

//
// forward declarations
//
template<class T>
class QFuture;
template<class T>
class QFutureInterface;
template<class T>
class QPromise;

namespace QtFuture {

enum class Launch { Sync, Async, Inherit };

template<class T>
struct WhenAnyResult
{
    qsizetype index = -1;
    QFuture<T> future;
};

// Deduction guide
template<class T>
WhenAnyResult(qsizetype, const QFuture<T> &) -> WhenAnyResult<T>;
}

namespace QtPrivate {

// implemented in qfutureinterface.cpp
Q_CORE_EXPORT void qfutureWarnIfUnusedResults(qsizetype numResults);

template<class T>
using EnableForVoid = std::enable_if_t<std::is_same_v<T, void>>;

template<class T>
using EnableForNonVoid = std::enable_if_t<!std::is_same_v<T, void>>;

template<typename F, typename Arg, typename Enable = void>
struct ResultTypeHelper
{
};

// The callable takes an argument of type Arg
template<typename F, typename Arg>
struct ResultTypeHelper<
        F, Arg, typename std::enable_if_t<!std::is_invocable_v<std::decay_t<F>, QFuture<Arg>>>>
{
    using ResultType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Arg>>;
};

// The callable takes an argument of type QFuture<Arg>
template<class F, class Arg>
struct ResultTypeHelper<
        F, Arg, typename std::enable_if_t<std::is_invocable_v<std::decay_t<F>, QFuture<Arg>>>>
{
    using ResultType = std::invoke_result_t<std::decay_t<F>, QFuture<Arg>>;
};

// The callable takes an argument of type QFuture<void>
template<class F>
struct ResultTypeHelper<
        F, void, typename std::enable_if_t<std::is_invocable_v<std::decay_t<F>, QFuture<void>>>>
{
    using ResultType = std::invoke_result_t<std::decay_t<F>, QFuture<void>>;
};

// The callable doesn't take argument
template<class F>
struct ResultTypeHelper<
        F, void, typename std::enable_if_t<!std::is_invocable_v<std::decay_t<F>, QFuture<void>>>>
{
    using ResultType = std::invoke_result_t<std::decay_t<F>>;
};

// Helpers to remove QPrivateSignal argument from the list of arguments

template<class T, class Enable = void>
inline constexpr bool IsPrivateSignalArg = false;

template<class T>
inline constexpr bool IsPrivateSignalArg<T, typename std::enable_if_t<
        // finds injected-class-name, the 'class' avoids falling into the rules of [class.qual]/2:
        std::is_class_v<class T::QPrivateSignal>
    >> = true;

template<class Tuple, std::size_t... I>
auto cutTuple(Tuple &&t, std::index_sequence<I...>)
{
    return std::make_tuple(std::get<I>(t)...);
}

template<class Arg, class... Args>
auto createTuple(Arg &&arg, Args &&... args)
{
    using TupleType = std::tuple<std::decay_t<Arg>, std::decay_t<Args>...>;
    constexpr auto Size = sizeof...(Args); // One less than the size of all arguments
    if constexpr (QtPrivate::IsPrivateSignalArg<std::tuple_element_t<Size, TupleType>>) {
        if constexpr (Size == 1) {
            return std::forward<Arg>(arg);
        } else {
            return cutTuple(std::make_tuple(std::forward<Arg>(arg), std::forward<Args>(args)...),
                            std::make_index_sequence<Size>());
        }
    } else {
        return std::make_tuple(std::forward<Arg>(arg), std::forward<Args>(args)...);
    }
}

// Helpers to resolve argument types of callables.

template<class Arg, class... Args>
using FilterLastPrivateSignalArg =
        std::conditional_t<(sizeof...(Args) > 0),
                           std::invoke_result_t<decltype(createTuple<Arg, Args...>), Arg, Args...>,
                           std::conditional_t<IsPrivateSignalArg<Arg>, void, Arg>>;

template<typename...>
struct ArgsType;

template<typename Arg, typename... Args>
struct ArgsType<Arg, Args...>
{
    using First = Arg;
    using PromiseType = void;
    using IsPromise = std::false_type;
    static const bool HasExtraArgs = (sizeof...(Args) > 0);
    using AllArgs = FilterLastPrivateSignalArg<std::decay_t<Arg>, std::decay_t<Args>...>;

    template<class Class, class Callable>
    static const bool CanInvokeWithArgs = std::is_invocable_v<Callable, Class, Arg, Args...>;
};

template<typename Arg, typename... Args>
struct ArgsType<QPromise<Arg> &, Args...>
{
    using First = QPromise<Arg> &;
    using PromiseType = Arg;
    using IsPromise = std::true_type;
    static const bool HasExtraArgs = (sizeof...(Args) > 0);
    using AllArgs = FilterLastPrivateSignalArg<QPromise<Arg>, std::decay_t<Args>...>;

    template<class Class, class Callable>
    static const bool CanInvokeWithArgs = std::is_invocable_v<Callable, Class, QPromise<Arg> &, Args...>;
};

template<>
struct ArgsType<>
{
    using First = void;
    using PromiseType = void;
    using IsPromise = std::false_type;
    static const bool HasExtraArgs = false;
    using AllArgs = void;

    template<class Class, class Callable>
    static const bool CanInvokeWithArgs = std::is_invocable_v<Callable, Class>;
};

template<typename F>
struct ArgResolver : ArgResolver<decltype(&std::decay_t<F>::operator())>
{
};

template<typename F>
struct ArgResolver<std::reference_wrapper<F>> : ArgResolver<decltype(&std::decay_t<F>::operator())>
{
};

template<typename R, typename... Args>
struct ArgResolver<R(Args...)> : public ArgsType<Args...>
{
};

template<typename R, typename... Args>
struct ArgResolver<R (*)(Args...)> : public ArgsType<Args...>
{
};

template<typename R, typename... Args>
struct ArgResolver<R (*&)(Args...)> : public ArgsType<Args...>
{
};

template<typename R, typename... Args>
struct ArgResolver<R (* const)(Args...)> : public ArgsType<Args...>
{
};

template<typename R, typename... Args>
struct ArgResolver<R (&)(Args...)> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::*)(Args...)> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::*)(Args...) noexcept> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::*)(Args...) const> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::*)(Args...) const noexcept> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::* const)(Args...) const> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::* const)(Args...) const noexcept> : public ArgsType<Args...>
{
};

template<class Class, class Callable>
using EnableIfInvocable = std::enable_if_t<
        QtPrivate::ArgResolver<Callable>::template CanInvokeWithArgs<Class, Callable>>;

template<class T>
inline constexpr bool isQFutureV = false;

template<class T>
inline constexpr bool isQFutureV<QFuture<T>> = true;

template<class T>
using isQFuture = std::bool_constant<isQFutureV<T>>;

template<class T>
struct Future
{
};

template<class T>
struct Future<QFuture<T>>
{
    using type = T;
};

template<class... Args>
using NotEmpty = std::bool_constant<(sizeof...(Args) > 0)>;

template<class Sequence>
using IsRandomAccessible =
        std::is_convertible<typename std::iterator_traits<std::decay_t<decltype(
                                    std::begin(std::declval<Sequence>()))>>::iterator_category,
                            std::random_access_iterator_tag>;

template<class Sequence>
using HasInputIterator =
        std::is_convertible<typename std::iterator_traits<std::decay_t<decltype(
                                    std::begin(std::declval<Sequence>()))>>::iterator_category,
                            std::input_iterator_tag>;

template<class Iterator>
using IsForwardIterable =
        std::is_convertible<typename std::iterator_traits<Iterator>::iterator_category,
                            std::forward_iterator_tag>;

template<typename Function, typename ResultType, typename ParentResultType>
class CompactContinuation : private CompactStorage<Function>
{
    Q_DISABLE_COPY_MOVE(CompactContinuation)
public:
    using Storage = CompactStorage<Function>;

    template<typename F = Function>
    CompactContinuation(F &&func, const QFuture<ParentResultType> &f, QPromise<ResultType> &&p)
        : Storage{std::forward<F>(func)}, promise(std::move(p)), parentFuture(f), type(Type::Sync)
    {
    }

    template<typename F = Function>
    CompactContinuation(F &&func, const QFuture<ParentResultType> &f, QPromise<ResultType> &&p,
                 QThreadPool *pool)
        : Storage{std::forward<F>(func)}, promise(std::move(p)), parentFuture(f),
          threadPool(pool), type(Type::Async)
    {
        runObj = QRunnable::create([continuation = this] {
            continuation->runFunction();
            delete continuation;
        });
        runObj->setAutoDelete(false);
    }

    ~CompactContinuation() { delete runObj; }

    bool execute();

    QRunnable *runnable() const { return runObj; }

    template<typename F = Function>
    static void create(F &&func, QFuture<ParentResultType> *f, QFutureInterface<ResultType> &fi,
                       QtFuture::Launch policy);

    template<typename F = Function>
    static void create(F &&func, QFuture<ParentResultType> *f, QFutureInterface<ResultType> &fi,
                       QThreadPool *pool);

    template<typename F = Function>
    static void create(F &&func, QFuture<ParentResultType> *f, QFutureInterface<ResultType> &fi,
                       QObject *context);

private:
    void fulfillPromiseWithResult();
    void fulfillVoidPromise();
    void fulfillPromiseWithVoidResult();

    template<class... Args>
    void fulfillPromise(Args &&... args);

protected:
    void runImpl()
    {
        if (type == Type::Sync) {
            runFunction();
        } else {
            Q_ASSERT(runObj);
            QThreadPool *pool = threadPool ? threadPool : QThreadPool::globalInstance();
            pool->start(runObj);
        }
    }

    void runFunction();

protected:
    enum class Type : quint8 {
        Sync,
        Async
    };

    QPromise<ResultType> promise;
    QFuture<ParentResultType> parentFuture;
    QThreadPool *threadPool = nullptr;
    QRunnable *runObj = nullptr;
    const Type type;
};

#ifndef QT_NO_EXCEPTIONS

template<class Function, class ResultType>
class FailureHandler
{
public:
    template<typename F = Function>
    static void create(F &&function, QFuture<ResultType> *future,
                       const QFutureInterface<ResultType> &fi);

    template<typename F = Function>
    static void create(F &&function, QFuture<ResultType> *future, QFutureInterface<ResultType> &fi,
                       QObject *context);

    template<typename F = Function>
    FailureHandler(F &&func, const QFuture<ResultType> &f, QPromise<ResultType> &&p)
        : promise(std::move(p)), parentFuture(f), handler(std::forward<F>(func))
    {
    }

public:
    void run();

private:
    template<class ArgType>
    void handleException();
    void handleAllExceptions();

private:
    QPromise<ResultType> promise;
    QFuture<ResultType> parentFuture;
    Function handler;
};

#endif

template<typename Function, typename ResultType, typename ParentResultType>
void CompactContinuation<Function, ResultType, ParentResultType>::runFunction()
{
    promise.start();

    Q_ASSERT(parentFuture.isFinished());

#ifndef QT_NO_EXCEPTIONS
    try {
#endif
        if constexpr (!std::is_void_v<ResultType>) {
            if constexpr (std::is_void_v<ParentResultType>) {
                fulfillPromiseWithVoidResult();
            } else if constexpr (std::is_invocable_v<Function, ParentResultType>) {
                fulfillPromiseWithResult();
            } else {
                // This assert normally should never fail, this is to make sure
                // that nothing unexpected happened.
                static_assert(std::is_invocable_v<Function, QFuture<ParentResultType>>,
                              "The continuation is not invocable with the provided arguments");
                fulfillPromise(parentFuture);
            }
        } else {
            if constexpr (std::is_void_v<ParentResultType>) {
                if constexpr (std::is_invocable_v<Function, QFuture<void>>)
                    this->object()(parentFuture);
                else
                    this->object()();
            } else if constexpr (std::is_invocable_v<Function, ParentResultType>) {
                fulfillVoidPromise();
            } else {
                // This assert normally should never fail, this is to make sure
                // that nothing unexpected happened.
                static_assert(std::is_invocable_v<Function, QFuture<ParentResultType>>,
                              "The continuation is not invocable with the provided arguments");
                this->object()(parentFuture);
            }
        }
#ifndef QT_NO_EXCEPTIONS
    } catch (...) {
        promise.setException(std::current_exception());
    }
#endif
    promise.finish();
}

template<typename Function, typename ResultType, typename ParentResultType>
bool CompactContinuation<Function, ResultType, ParentResultType>::execute()
{
    Q_ASSERT(parentFuture.isFinished());

    if (parentFuture.d.isChainCanceled()) {
#ifndef QT_NO_EXCEPTIONS
        if (parentFuture.d.hasException()) {
            // If the continuation doesn't take a QFuture argument, propagate the exception
            // to the caller, by reporting it. If the continuation takes a QFuture argument,
            // the user may want to catch the exception inside the continuation, to not
            // interrupt the continuation chain, so don't report anything yet.
            if constexpr (!std::is_invocable_v<std::decay_t<Function>, QFuture<ParentResultType>>) {
                promise.start();
                promise.setException(parentFuture.d.exceptionStore().exception());
                promise.finish();
                return false;
            }
        } else
#endif
        {
            promise.start();
            promise.future().cancel();
            promise.finish();
            return false;
        }
    }

    runImpl();
    return true;
}

// Workaround for keeping move-only lambdas inside std::function
template<class Function>
struct ContinuationWrapper
{
    ContinuationWrapper(Function &&f) : function(std::move(f)) { }
    ContinuationWrapper(const ContinuationWrapper &other)
        : function(std::move(const_cast<ContinuationWrapper &>(other).function))
    {
        Q_ASSERT_X(false, "QFuture", "Continuation shouldn't be copied");
    }
    ContinuationWrapper(ContinuationWrapper &&other) = default;
    ContinuationWrapper &operator=(ContinuationWrapper &&) = default;

    template <typename F = Function,
              std::enable_if_t<std::is_invocable_v<F, const QFutureInterfaceBase &>, bool> = true>
    void operator()(const QFutureInterfaceBase &parentData) { function(parentData); }

    template <typename F = Function, std::enable_if_t<std::is_invocable_v<F>, bool> = true>
    void operator()() { function(); }

private:
    Function function;
};

template<typename Function, typename ResultType, typename ParentResultType>
template<typename F>
void CompactContinuation<Function, ResultType, ParentResultType>::create(F &&func,
                                                                  QFuture<ParentResultType> *f,
                                                                  QFutureInterface<ResultType> &fi,
                                                                  QtFuture::Launch policy)
{
    Q_ASSERT(f);

    QThreadPool *pool = nullptr;

    bool launchAsync = (policy == QtFuture::Launch::Async);
    if (policy == QtFuture::Launch::Inherit) {
        launchAsync = f->d.launchAsync();

        // If the parent future was using a custom thread pool, inherit it as well.
        if (launchAsync && f->d.threadPool()) {
            pool = f->d.threadPool();
            fi.setThreadPool(pool);
        }
    }

    fi.setLaunchAsync(launchAsync);

    auto continuation = [func = std::forward<F>(func), fi, promise_ = QPromise(fi), pool,
                         launchAsync](const QFutureInterfaceBase &parentData) mutable {
        const auto parent = QFutureInterface<ParentResultType>(parentData).future();
        CompactContinuation<Function, ResultType, ParentResultType> *continuationJob = nullptr;
        if (launchAsync) {
            auto asyncJob = new CompactContinuation<Function, ResultType, ParentResultType>(
                    std::forward<Function>(func), parent, std::move(promise_), pool);
            fi.setRunnable(asyncJob->runnable());
            continuationJob = asyncJob;
        } else {
            continuationJob = new CompactContinuation<Function, ResultType, ParentResultType>(
                    std::forward<Function>(func), parent, std::move(promise_));
        }

        bool isLaunched = continuationJob->execute();
        // If continuation is successfully launched, AsyncContinuation will be deleted
        // from the QRunnable's lambda. Synchronous continuation will be
        // executed immediately, so it's safe to always delete it here.
        if (!(launchAsync && isLaunched)) {
            delete continuationJob;
            continuationJob = nullptr;
        }
    };
    f->d.setContinuation(ContinuationWrapper(std::move(continuation)), fi.d,
                         QFutureInterfaceBase::ContinuationType::Then);
}

template<typename Function, typename ResultType, typename ParentResultType>
template<typename F>
void CompactContinuation<Function, ResultType, ParentResultType>::create(F &&func,
                                                                  QFuture<ParentResultType> *f,
                                                                  QFutureInterface<ResultType> &fi,
                                                                  QThreadPool *pool)
{
    Q_ASSERT(f);

    fi.setLaunchAsync(true);
    fi.setThreadPool(pool);

    auto continuation = [func = std::forward<F>(func), promise_ = QPromise(fi),
                         pool](const QFutureInterfaceBase &parentData) mutable {
        const auto parent = QFutureInterface<ParentResultType>(parentData).future();
        auto continuationJob = new CompactContinuation<Function, ResultType, ParentResultType>(
                std::forward<Function>(func), parent, std::move(promise_), pool);
        bool isLaunched = continuationJob->execute();
        // If continuation is successfully launched, AsyncContinuation will be deleted
        // from the QRunnable's lambda.
        if (!isLaunched) {
            delete continuationJob;
            continuationJob = nullptr;
        }
    };
    f->d.setContinuation(ContinuationWrapper(std::move(continuation)), fi.d,
                         QFutureInterfaceBase::ContinuationType::Then);
}

template<typename Function, typename ResultType, typename ParentResultType>
template<typename F>
void CompactContinuation<Function, ResultType, ParentResultType>::create(F &&func,
                                                                  QFuture<ParentResultType> *f,
                                                                  QFutureInterface<ResultType> &fi,
                                                                  QObject *context)
{
    Q_ASSERT(f);
    Q_ASSERT(context);

    // When the context object is destroyed, the signal-slot connection is broken and the
    // continuation callback is destroyed. The promise that is created in the capture list is
    // destroyed and, if it is not yet finished, cancelled.
    auto continuation = [func = std::forward<F>(func), parent = *f,
                         promise_ = QPromise(fi)]() mutable {
        CompactContinuation<Function, ResultType, ParentResultType> continuationJob(
                std::forward<Function>(func), parent, std::move(promise_));
        continuationJob.execute();
    };

    f->d.setContinuation(context, ContinuationWrapper(std::move(continuation)),
                         QVariant::fromValue(fi),
                         QFutureInterfaceBase::ContinuationType::Then);
}

template<typename Function, typename ResultType, typename ParentResultType>
void CompactContinuation<Function, ResultType, ParentResultType>::fulfillPromiseWithResult()
{
    qfutureWarnIfUnusedResults(parentFuture.resultCount());
    if constexpr (std::is_copy_constructible_v<ParentResultType>)
        fulfillPromise(parentFuture.result());
    else
        fulfillPromise(parentFuture.takeResult());
}

template<typename Function, typename ResultType, typename ParentResultType>
void CompactContinuation<Function, ResultType, ParentResultType>::fulfillVoidPromise()
{
    qfutureWarnIfUnusedResults(parentFuture.resultCount());
    if constexpr (std::is_copy_constructible_v<ParentResultType>)
        this->object()(parentFuture.result());
    else
        this->object()(parentFuture.takeResult());
}

template<typename Function, typename ResultType, typename ParentResultType>
void CompactContinuation<Function, ResultType, ParentResultType>::fulfillPromiseWithVoidResult()
{
    if constexpr (std::is_invocable_v<Function, QFuture<void>>)
        fulfillPromise(parentFuture);
    else
        fulfillPromise();
}

template<typename Function, typename ResultType, typename ParentResultType>
template<class... Args>
void CompactContinuation<Function, ResultType, ParentResultType>::fulfillPromise(Args &&... args)
{
    promise.addResult(std::invoke(this->object(), std::forward<Args>(args)...));
}

template<class T>
void fulfillPromise(QPromise<T> &promise, QFuture<T> &future)
{
    if constexpr (!std::is_void_v<T>) {
        if constexpr (std::is_copy_constructible_v<T>)
            promise.addResult(future.result());
        else
            promise.addResult(future.takeResult());
    }
}

template<class T, class Function>
void fulfillPromise(QPromise<T> &promise, Function &&handler)
{
    if constexpr (std::is_void_v<T>)
        handler();
    else
        promise.addResult(handler());
}

#ifndef QT_NO_EXCEPTIONS

template<class Function, class ResultType>
template<class F>
void FailureHandler<Function, ResultType>::create(F &&function, QFuture<ResultType> *future,
                                                  const QFutureInterface<ResultType> &fi)
{
    Q_ASSERT(future);

    auto failureContinuation = [function = std::forward<F>(function), promise_ = QPromise(fi)](
                                       const QFutureInterfaceBase &parentData) mutable {
        const auto parent = QFutureInterface<ResultType>(parentData).future();
        FailureHandler<Function, ResultType> failureHandler(std::forward<Function>(function),
                                                            parent, std::move(promise_));
        failureHandler.run();
    };

    future->d.setContinuation(ContinuationWrapper(std::move(failureContinuation)), fi.d,
                              QFutureInterfaceBase::ContinuationType::OnFailed);
}

template<class Function, class ResultType>
template<class F>
void FailureHandler<Function, ResultType>::create(F &&function, QFuture<ResultType> *future,
                                                  QFutureInterface<ResultType> &fi,
                                                  QObject *context)
{
    Q_ASSERT(future);
    Q_ASSERT(context);
    auto failureContinuation = [function = std::forward<F>(function),
                                parent = *future, promise_ = QPromise(fi)]() mutable {
        FailureHandler<Function, ResultType> failureHandler(
                std::forward<Function>(function), parent, std::move(promise_));
        failureHandler.run();
    };

    future->d.setContinuation(context, ContinuationWrapper(std::move(failureContinuation)),
                              QVariant::fromValue(fi),
                              QFutureInterfaceBase::ContinuationType::OnFailed);
}

template<class Function, class ResultType>
void FailureHandler<Function, ResultType>::run()
{
    Q_ASSERT(parentFuture.isFinished());

    promise.start();

    if (parentFuture.d.hasException()) {
        using ArgType = typename QtPrivate::ArgResolver<Function>::First;
        if constexpr (std::is_void_v<ArgType>) {
            handleAllExceptions();
        } else {
            handleException<ArgType>();
        }
    } else if (parentFuture.d.isChainCanceled()) {
        promise.future().cancel();
    } else {
        QtPrivate::fulfillPromise(promise, parentFuture);
    }
    promise.finish();
}

template<class Function, class ResultType>
template<class ArgType>
void FailureHandler<Function, ResultType>::handleException()
{
    try {
        Q_ASSERT(parentFuture.d.hasException());
        parentFuture.d.exceptionStore().rethrowException();
    } catch (const ArgType &e) {
        try {
            // Handle exceptions matching with the handler's argument type
            if constexpr (std::is_void_v<ResultType>)
                handler(e);
            else
                promise.addResult(handler(e));
        } catch (...) {
            promise.setException(std::current_exception());
        }
    } catch (...) {
        // Exception doesn't match with handler's argument type, propagate
        // the exception to be handled later.
        promise.setException(std::current_exception());
    }
}

template<class Function, class ResultType>
void FailureHandler<Function, ResultType>::handleAllExceptions()
{
    try {
        Q_ASSERT(parentFuture.d.hasException());
        parentFuture.d.exceptionStore().rethrowException();
    } catch (...) {
        try {
            QtPrivate::fulfillPromise(promise, std::forward<Function>(handler));
        } catch (...) {
            promise.setException(std::current_exception());
        }
    }
}

#endif // QT_NO_EXCEPTIONS

template<class Function, class ResultType>
class CanceledHandler
{
public:
    template<class F = Function>
    static void create(F &&handler, QFuture<ResultType> *future, QFutureInterface<ResultType> &fi)
    {
        Q_ASSERT(future);

        auto canceledContinuation = [promise = QPromise(fi), handler = std::forward<F>(handler)](
                                            const QFutureInterfaceBase &parentData) mutable {
            auto parentFuture = QFutureInterface<ResultType>(parentData).future();
            run(std::forward<F>(handler), parentFuture, std::move(promise));
        };
        future->d.setContinuation(ContinuationWrapper(std::move(canceledContinuation)), fi.d,
                                  QFutureInterfaceBase::ContinuationType::OnCanceled);
    }

    template<class F = Function>
    static void create(F &&handler, QFuture<ResultType> *future, QFutureInterface<ResultType> &fi,
                       QObject *context)
    {
        Q_ASSERT(future);
        Q_ASSERT(context);
        auto canceledContinuation = [handler = std::forward<F>(handler),
                                     parentFuture = *future, promise = QPromise(fi)]() mutable {
            run(std::forward<F>(handler), parentFuture, std::move(promise));
        };

        future->d.setContinuation(context, ContinuationWrapper(std::move(canceledContinuation)),
                                  QVariant::fromValue(fi),
                                  QFutureInterfaceBase::ContinuationType::OnCanceled);
    }

    template<class F = Function>
    static void run(F &&handler, QFuture<ResultType> &parentFuture, QPromise<ResultType> &&promise)
    {
        promise.start();

        if (parentFuture.isCanceled()) {
#ifndef QT_NO_EXCEPTIONS
            if (parentFuture.d.hasException()) {
                // Propagate the exception to the result future
                promise.setException(parentFuture.d.exceptionStore().exception());
            } else {
                try {
#endif
                    QtPrivate::fulfillPromise(promise, std::forward<F>(handler));
#ifndef QT_NO_EXCEPTIONS
                } catch (...) {
                    promise.setException(std::current_exception());
                }
            }
#endif
        } else {
            QtPrivate::fulfillPromise(promise, parentFuture);
        }

        promise.finish();
    }
};

struct UnwrapHandler
{
    template<class T>
    static auto unwrapImpl(T *outer)
    {
        Q_ASSERT(outer);

        using ResultType = typename QtPrivate::Future<std::decay_t<T>>::type;
        using NestedType = typename QtPrivate::Future<ResultType>::type;
        QFutureInterface<NestedType> promise(QFutureInterfaceBase::State::Pending);

        auto chain = outer->then([promise](const QFuture<ResultType> &outerFuture) mutable {
            // We use the .then([](QFuture<ResultType> outerFuture) {...}) version
            // (where outerFuture == *outer), to propagate the exception if the
            // outer future has failed.
            Q_ASSERT(outerFuture.isFinished());
#ifndef QT_NO_EXCEPTIONS
            if (outerFuture.d.hasException()) {
                promise.reportStarted();
                promise.reportException(outerFuture.d.exceptionStore().exception());
                promise.reportFinished();
                return;
            }
#endif

            promise.reportStarted();
            ResultType nestedFuture = outerFuture.result();

            nestedFuture.then([promise] (const QFuture<NestedType> &nested) mutable {
#ifndef QT_NO_EXCEPTIONS
                if (nested.d.hasException()) {
                    promise.reportException(nested.d.exceptionStore().exception());
                } else
#endif
                {
                    if constexpr (!std::is_void_v<NestedType>)
                        promise.reportResults(nested.results());
                }
                promise.reportFinished();
            }).onCanceled([promise] () mutable {
                promise.reportCanceled();
                promise.reportFinished();
            });
        }).onCanceled([promise]() mutable {
            // propagate the cancellation of the outer future
            promise.reportStarted();
            promise.reportCanceled();
            promise.reportFinished();
        });

        // Inject the promise into the chain.
        // We use a fake function as a continuation, since the promise is
        // managed by the outer future
        chain.d.setContinuation(ContinuationWrapper(std::move([](const QFutureInterfaceBase &) {})),
                                promise.d, QFutureInterfaceBase::ContinuationType::Then);

        return promise.future();
    }
};

template<typename ValueType>
QFuture<ValueType> makeReadyRangeFutureImpl(const QList<ValueType> &values)
{
    QFutureInterface<ValueType> promise;
    promise.reportStarted();
    promise.reportResults(values);
    promise.reportFinished();
    return promise.future();
}

} // namespace QtPrivate

namespace QtFuture {

template<class Signal>
using ArgsType = typename QtPrivate::ArgResolver<Signal>::AllArgs;

template<class Sender, class Signal, typename = QtPrivate::EnableIfInvocable<Sender, Signal>>
static QFuture<ArgsType<Signal>> connect(Sender *sender, Signal signal)
{
    using ArgsType = ArgsType<Signal>;
    QFutureInterface<ArgsType> promise;
    promise.reportStarted();
    if (!sender) {
        promise.reportCanceled();
        promise.reportFinished();
        return promise.future();
    }

    using Connections = std::pair<QMetaObject::Connection, QMetaObject::Connection>;
    auto connections = std::make_shared<Connections>();

    if constexpr (std::is_void_v<ArgsType>) {
        connections->first =
                QObject::connect(sender, signal, sender, [promise, connections]() mutable {
                    QObject::disconnect(connections->first);
                    QObject::disconnect(connections->second);
                    promise.reportFinished();
                });
    } else if constexpr (QtPrivate::ArgResolver<Signal>::HasExtraArgs) {
        connections->first = QObject::connect(sender, signal, sender,
                                              [promise, connections](auto... values) mutable {
                                                  QObject::disconnect(connections->first);
                                                  QObject::disconnect(connections->second);
                                                  promise.reportResult(QtPrivate::createTuple(
                                                          std::move(values)...));
                                                  promise.reportFinished();
                                              });
    } else {
        connections->first = QObject::connect(sender, signal, sender,
                                              [promise, connections](ArgsType value) mutable {
                                                  QObject::disconnect(connections->first);
                                                  QObject::disconnect(connections->second);
                                                  promise.reportResult(value);
                                                  promise.reportFinished();
                                              });
    }

    if (!connections->first) {
        promise.reportCanceled();
        promise.reportFinished();
        return promise.future();
    }

    connections->second =
            QObject::connect(sender, &QObject::destroyed, sender, [promise, connections]() mutable {
                QObject::disconnect(connections->first);
                QObject::disconnect(connections->second);
                promise.reportCanceled();
                promise.reportFinished();
            });

    return promise.future();
}

template<typename Container>
using if_container_with_input_iterators =
        std::enable_if_t<QtPrivate::HasInputIterator<Container>::value, bool>;

template<typename Container>
using ContainedType =
        typename std::iterator_traits<decltype(
                    std::cbegin(std::declval<Container&>()))>::value_type;

template<typename Container, if_container_with_input_iterators<Container> = true>
static QFuture<ContainedType<Container>> makeReadyRangeFuture(Container &&container)
{
    // handle QList<T> separately, because reportResults() takes a QList
    // as an input
    using ValueType = ContainedType<Container>;
    if constexpr (std::is_convertible_v<q20::remove_cvref_t<Container>, QList<ValueType>>) {
        return QtPrivate::makeReadyRangeFutureImpl(container);
    } else {
        return QtPrivate::makeReadyRangeFutureImpl(QList<ValueType>{std::cbegin(container),
                                                                    std::cend(container)});
    }
}

template<typename ValueType>
static QFuture<ValueType> makeReadyRangeFuture(std::initializer_list<ValueType> values)
{
    return QtPrivate::makeReadyRangeFutureImpl(QList<ValueType>{values});
}

template<typename T>
static QFuture<std::decay_t<T>> makeReadyValueFuture(T &&value)
{
    QFutureInterface<std::decay_t<T>> promise;
    promise.reportStarted();
    promise.reportResult(std::forward<T>(value));
    promise.reportFinished();

    return promise.future();
}

Q_CORE_EXPORT QFuture<void> makeReadyVoidFuture(); // implemented in qfutureinterface.cpp

#if QT_DEPRECATED_SINCE(6, 10)
template<typename T, typename = QtPrivate::EnableForNonVoid<T>>
QT_DEPRECATED_VERSION_X(6, 10, "Use makeReadyValueFuture() instead.")
static QFuture<std::decay_t<T>> makeReadyFuture(T &&value)
{
    return makeReadyValueFuture(std::forward<T>(value));
}

// the void specialization is moved to the end of qfuture.h, because it now
// uses makeReadyVoidFuture() and required QFuture<void> to be defined.

template<typename T>
QT_DEPRECATED_VERSION_X(6, 10, "Use makeReadyRangeFuture() instead.")
static QFuture<T> makeReadyFuture(const QList<T> &values)
{
    return makeReadyRangeFuture(values);
}
#endif // QT_DEPRECATED_SINCE(6, 10)

#ifndef QT_NO_EXCEPTIONS

template<typename T = void>
static QFuture<T> makeExceptionalFuture(std::exception_ptr exception)
{
    QFutureInterface<T> promise;
    promise.reportStarted();
    promise.reportException(exception);
    promise.reportFinished();

    return promise.future();
}

template<typename T = void>
static QFuture<T> makeExceptionalFuture(const QException &exception)
{
    try {
        exception.raise();
    } catch (...) {
        return makeExceptionalFuture<T>(std::current_exception());
    }
    Q_UNREACHABLE();
}

#endif // QT_NO_EXCEPTIONS

} // namespace QtFuture

namespace QtPrivate {

template<typename ResultFutures>
struct WhenAllContext
{
    using ValueType = typename ResultFutures::value_type;

    explicit WhenAllContext(qsizetype size) : remaining(size) {}

    template<typename T = ValueType>
    void checkForCompletion(qsizetype index, T &&future)
    {
        futures[index] = std::forward<T>(future);
        const auto oldRemaining = remaining.fetchAndSubRelaxed(1);
        Q_ASSERT(oldRemaining > 0);
        if (oldRemaining <= 1) { // that was the last one
            promise.addResult(futures);
            promise.finish();
        }
    }

    QAtomicInteger<qsizetype> remaining;
    QPromise<ResultFutures> promise;
    ResultFutures futures;
};

template<typename ResultType>
struct WhenAnyContext
{
    using ValueType = ResultType;

    template<typename T = ResultType, typename = EnableForNonVoid<T>>
    void checkForCompletion(qsizetype, T &&result)
    {
        if (!ready.fetchAndStoreRelaxed(true)) {
            promise.addResult(std::forward<T>(result));
            promise.finish();
        }
    }

    QAtomicInt ready = false;
    QPromise<ResultType> promise;
};

template<qsizetype Index, typename ContextType, typename... Ts>
void addCompletionHandlersImpl(const std::shared_ptr<ContextType> &context,
                               const std::tuple<Ts...> &t)
{
    auto future = std::get<Index>(t);
    using ResultType = typename ContextType::ValueType;
    // Need context=context so that the compiler does not infer the captured variable's type as 'const'
    future.then([context=context](const std::tuple_element_t<Index, std::tuple<Ts...>> &f) {
        context->checkForCompletion(Index, ResultType { std::in_place_index<Index>, f });
    }).onCanceled([context=context, future]() {
        context->checkForCompletion(Index, ResultType { std::in_place_index<Index>, future });
    });

    if constexpr (Index != 0)
        addCompletionHandlersImpl<Index - 1, ContextType, Ts...>(context, t);
}

template<typename ContextType, typename... Ts>
void addCompletionHandlers(const std::shared_ptr<ContextType> &context, const std::tuple<Ts...> &t)
{
    constexpr qsizetype size = std::tuple_size<std::tuple<Ts...>>::value;
    addCompletionHandlersImpl<size - 1, ContextType, Ts...>(context, t);
}

template<typename OutputSequence, typename InputIt, typename ValueType,
         std::enable_if_t<std::conjunction_v<IsForwardIterable<InputIt>, isQFuture<ValueType>>,
                          bool> = true>
QFuture<OutputSequence> whenAllImpl(InputIt first, InputIt last)
{
    const qsizetype size = std::distance(first, last);
    if (size == 0)
        return QtFuture::makeReadyValueFuture(OutputSequence());

    const auto context = std::make_shared<QtPrivate::WhenAllContext<OutputSequence>>(size);
    context->futures.resize(size);
    context->promise.start();

    qsizetype idx = 0;
    for (auto it = first; it != last; ++it, ++idx) {
        // Need context=context so that the compiler does not infer the captured variable's type as 'const'
        it->then([context=context, idx](const ValueType &f) {
            context->checkForCompletion(idx, f);
        }).onCanceled([context=context, idx, f = *it] {
            context->checkForCompletion(idx, f);
        });
    }
    return context->promise.future();
}

template<typename OutputSequence, typename... Futures>
QFuture<OutputSequence> whenAllImpl(Futures &&... futures)
{
    constexpr qsizetype size = sizeof...(Futures);
    const auto context = std::make_shared<QtPrivate::WhenAllContext<OutputSequence>>(size);
    context->futures.resize(size);
    context->promise.start();

    QtPrivate::addCompletionHandlers(context, std::make_tuple(std::forward<Futures>(futures)...));

    return context->promise.future();
}

template<typename InputIt, typename ValueType,
         std::enable_if_t<std::conjunction_v<IsForwardIterable<InputIt>, isQFuture<ValueType>>,
                          bool> = true>
QFuture<QtFuture::WhenAnyResult<typename Future<ValueType>::type>> whenAnyImpl(InputIt first,
                                                                               InputIt last)
{
    using PackagedType = typename Future<ValueType>::type;
    using ResultType = QtFuture::WhenAnyResult<PackagedType>;

    const qsizetype size = std::distance(first, last);
    if (size == 0) {
        return QtFuture::makeReadyValueFuture(
                QtFuture::WhenAnyResult { qsizetype(-1), QFuture<PackagedType>() });
    }

    const auto context = std::make_shared<QtPrivate::WhenAnyContext<ResultType>>();
    context->promise.start();

    qsizetype idx = 0;
    for (auto it = first; it != last; ++it, ++idx) {
        // Need context=context so that the compiler does not infer the captured variable's type as 'const'
        it->then([context=context, idx](const ValueType &f) {
            context->checkForCompletion(idx, QtFuture::WhenAnyResult { idx, f });
        }).onCanceled([context=context, idx, f = *it] {
            context->checkForCompletion(idx, QtFuture::WhenAnyResult { idx, f });
        });
    }
    return context->promise.future();
}

template<typename... Futures>
QFuture<std::variant<std::decay_t<Futures>...>> whenAnyImpl(Futures &&... futures)
{
    using ResultType = std::variant<std::decay_t<Futures>...>;

    const auto context = std::make_shared<QtPrivate::WhenAnyContext<ResultType>>();
    context->promise.start();

    QtPrivate::addCompletionHandlers(context, std::make_tuple(std::forward<Futures>(futures)...));

    return context->promise.future();
}

} // namespace QtPrivate

QT_END_NAMESPACE
