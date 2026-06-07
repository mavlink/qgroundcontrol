// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAPPLICATIONSTATIC_H
#define QAPPLICATIONSTATIC_H

#include <QtCore/QMutex>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qglobalstatic.h>

#include <new>

#if 0
#pragma qt_class(QApplicationStatic)
#endif

QT_BEGIN_NAMESPACE

namespace QtGlobalStatic {
template <typename QAS> struct ApplicationHolder
{
    using Type = typename QAS::QAS_Type;
    using PlainType = std::remove_cv_t<Type>;

    Q_CONSTINIT static inline struct { alignas(Type) unsigned char data[sizeof(Type)]; } storage = {};
    Q_CONSTINIT static inline QBasicAtomicInteger<qint8> guard = { QtGlobalStatic::Uninitialized };
    Q_CONSTINIT static inline QBasicMutex mutex {};

    static constexpr bool MutexLockIsNoexcept = noexcept(mutex.lock());
    static constexpr bool ConstructionIsNoexcept = noexcept(QAS::innerFunction(nullptr));

    ApplicationHolder() = default;
    Q_DISABLE_COPY_MOVE(ApplicationHolder)
    ~ApplicationHolder()
    {
        if (guard.loadAcquire() == QtGlobalStatic::Initialized) {
            // No mutex! Up to external code to ensure no race happens.
            guard.storeRelease(QtGlobalStatic::Destroyed);
            realPointer()->~PlainType();
        }
    }

    static PlainType *realPointer()
    {
        return std::launder(reinterpret_cast<PlainType *>(&storage));
    }

    // called from QGlobalStatic::instance()
    PlainType *pointer() noexcept(MutexLockIsNoexcept && ConstructionIsNoexcept)
    {
        if (guard.loadAcquire() == QtGlobalStatic::Initialized)
            return realPointer();
        QMutexLocker locker(&mutex);
        if (guard.loadRelaxed() == QtGlobalStatic::Uninitialized) {
            QAS::innerFunction(&storage);
            const auto *app = QCoreApplication::instance();
            Q_ASSERT_X(app, Q_FUNC_INFO,
                       "The application static was used without a QCoreApplication instance");
            QObject::connect(app, &QObject::destroyed, app, reset, Qt::DirectConnection);
            guard.storeRelease(QtGlobalStatic::Initialized);
        }
        return realPointer();
    }

    static void reset()
    {
        // we only synchronize using the mutex here, not the guard
        QMutexLocker locker(&mutex);
        realPointer()->~PlainType();
        guard.storeRelaxed(QtGlobalStatic::Uninitialized);
    }
};
} // namespace QtGlobalStatic

#define Q_APPLICATION_STATIC(TYPE, NAME, ...) \
    namespace { struct Q_QAS_ ## NAME {                                     \
        typedef TYPE QAS_Type;                                              \
        static void innerFunction(void *pointer)                            \
            noexcept(noexcept(std::remove_cv_t<QAS_Type>(__VA_ARGS__)))     \
        {                                                                   \
            new (pointer) QAS_Type(__VA_ARGS__);                            \
        }                                                                   \
    }; }                                                                    \
    static QGlobalStatic<QtGlobalStatic::ApplicationHolder<Q_QAS_ ## NAME>> NAME;\
    /**/

QT_END_NAMESPACE

#endif // QAPPLICATIONSTATIC_H
