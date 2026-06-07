// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLOBALSTATIC_H
#define QGLOBALSTATIC_H

#include <QtCore/qassert.h>
#include <QtCore/qatomic.h>
#include <QtCore/qtclasshelpermacros.h>

#include <atomic>           // for bootstrapped (no thread) builds
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtGlobalStatic {
enum GuardValues {
    Destroyed = -2,
    Initialized = -1,
    Uninitialized = 0,
    Initializing = 1
};

template <typename QGS> union Holder
{
    using Type = typename QGS::QGS_Type;
    using PlainType = std::remove_cv_t<Type>;

    static constexpr bool ConstructionIsNoexcept = noexcept(QGS::innerFunction(nullptr));
    Q_CONSTINIT static inline QBasicAtomicInteger<qint8> guard = { QtGlobalStatic::Uninitialized };

    // union's sole member
    PlainType storage;

    Holder() noexcept(ConstructionIsNoexcept)
    {
        QGS::innerFunction(pointer());
        guard.storeRelaxed(QtGlobalStatic::Initialized);
    }

    ~Holder()
    {
        // TSAN does not support atomic_thread_fence and GCC complains:
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97868
        // https://github.com/google/sanitizers/issues/1352
        // QTBUG-134415
QT_WARNING_PUSH
#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 1100
QT_WARNING_DISABLE_GCC("-Wtsan")
#endif
        // import changes to *pointer() by other threads before running ~PlainType():
        std::atomic_thread_fence(std::memory_order_acquire);
QT_WARNING_POP
        pointer()->~PlainType();
        guard.storeRelease(QtGlobalStatic::Destroyed);
    }

    PlainType *pointer() noexcept
    {
        return &storage;
    }

    Q_DISABLE_COPY_MOVE(Holder)
};
}

template <typename Holder> struct QGlobalStatic
{
    using Type = typename Holder::Type;

    bool isDestroyed() const noexcept { return guardValue() <= QtGlobalStatic::Destroyed; }
    bool exists() const noexcept { return guardValue() == QtGlobalStatic::Initialized; }
    operator Type *()
    {
        if (isDestroyed())
            return nullptr;
        return instance();
    }
    Type *operator()()
    {
        if (isDestroyed())
            return nullptr;
        return instance();
    }
    Type *operator->()
    {
        Q_ASSERT_X(!isDestroyed(), Q_FUNC_INFO,
                   "The global static was used after being destroyed");
        return instance();
    }
    Type &operator*()
    {
        Q_ASSERT_X(!isDestroyed(), Q_FUNC_INFO,
                   "The global static was used after being destroyed");
        return *instance();
    }

protected:
    static Type *instance() noexcept(Holder::ConstructionIsNoexcept)
    {
        static Holder holder;
        return holder.pointer();
    }
    static QtGlobalStatic::GuardValues guardValue() noexcept
    {
        return QtGlobalStatic::GuardValues(Holder::guard.loadAcquire());
    }
};

#define Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ARGS)                         \
    QT_WARNING_PUSH                                                         \
    QT_WARNING_DISABLE_CLANG("-Wunevaluated-expression")                    \
    namespace { struct Q_QGS_ ## NAME {                                     \
        typedef TYPE QGS_Type;                                              \
        static void innerFunction(void *pointer)                            \
            noexcept(noexcept(std::remove_cv_t<QGS_Type> ARGS))             \
        {                                                                   \
            new (pointer) QGS_Type ARGS;                                    \
        }                                                                   \
    }; }                                                                    \
    Q_CONSTINIT static QGlobalStatic<QtGlobalStatic::Holder<Q_QGS_ ## NAME>> NAME; \
    QT_WARNING_POP
    /**/

#define Q_GLOBAL_STATIC(TYPE, NAME, ...)                                    \
    Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, (__VA_ARGS__))

QT_END_NAMESPACE
#endif // QGLOBALSTATIC_H
