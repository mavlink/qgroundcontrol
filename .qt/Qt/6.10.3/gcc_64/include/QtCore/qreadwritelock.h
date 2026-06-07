// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QREADWRITELOCK_H
#define QREADWRITELOCK_H

#include <QtCore/qglobal.h>
#include <QtCore/qdeadlinetimer.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(thread)

class QReadWriteLockPrivate;

class Q_CORE_EXPORT QReadWriteLock
{
public:
    enum RecursionMode { NonRecursive, Recursive };

    QT_CORE_INLINE_SINCE(6, 6)
    explicit QReadWriteLock(RecursionMode recursionMode = NonRecursive);
    QT_CORE_INLINE_SINCE(6, 6)
    ~QReadWriteLock();

    QT_CORE_INLINE_SINCE(6, 6)
    void lockForRead();
#if QT_CORE_REMOVED_SINCE(6, 6)
    bool tryLockForRead();
#endif
    QT_CORE_INLINE_SINCE(6, 6)
    bool tryLockForRead(int timeout);
    bool tryLockForRead(QDeadlineTimer timeout = {});

    QT_CORE_INLINE_SINCE(6, 6)
    void lockForWrite();
#if QT_CORE_REMOVED_SINCE(6, 6)
    bool tryLockForWrite();
#endif
    QT_CORE_INLINE_SINCE(6, 6)
    bool tryLockForWrite(int timeout);
    bool tryLockForWrite(QDeadlineTimer timeout = {});

    void unlock();

private:
    Q_DISABLE_COPY(QReadWriteLock)
    QAtomicPointer<QReadWriteLockPrivate> d_ptr;
    friend class QReadWriteLockPrivate;
    static QReadWriteLockPrivate *initRecursive();
    static void destroyRecursive(QReadWriteLockPrivate *);
};

#if QT_CORE_INLINE_IMPL_SINCE(6, 6)
QReadWriteLock::QReadWriteLock(RecursionMode recursionMode)
    : d_ptr(recursionMode == Recursive ? initRecursive() : nullptr)
{
}

QReadWriteLock::~QReadWriteLock()
{
    if (auto d = d_ptr.loadAcquire())
        destroyRecursive(d);
}

void QReadWriteLock::lockForRead()
{
    tryLockForRead(QDeadlineTimer(QDeadlineTimer::Forever));
}

bool QReadWriteLock::tryLockForRead(int timeout)
{
    return tryLockForRead(QDeadlineTimer(timeout));
}

void QReadWriteLock::lockForWrite()
{
    tryLockForWrite(QDeadlineTimer(QDeadlineTimer::Forever));
}

bool QReadWriteLock::tryLockForWrite(int timeout)
{
    return tryLockForWrite(QDeadlineTimer(timeout));
}
#endif // inline since 6.6

#if defined(Q_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4312 ) // ignoring the warning from /Wp64
#endif

class QT6_ONLY(Q_CORE_EXPORT) QReadLocker
{
public:
    Q_NODISCARD_CTOR
    inline QReadLocker(QReadWriteLock *readWriteLock);

    inline ~QReadLocker()
    { unlock(); }

    inline void unlock()
    {
        if (q_val) {
            if ((q_val & quintptr(1u)) == quintptr(1u)) {
                q_val &= ~quintptr(1u);
                readWriteLock()->unlock();
            }
        }
    }

    inline void relock()
    {
        if (q_val) {
            if ((q_val & quintptr(1u)) == quintptr(0u)) {
                readWriteLock()->lockForRead();
                q_val |= quintptr(1u);
            }
        }
    }

    inline QReadWriteLock *readWriteLock() const
    { return reinterpret_cast<QReadWriteLock *>(q_val & ~quintptr(1u)); }

private:
    Q_DISABLE_COPY(QReadLocker)
    quintptr q_val;
};

inline QReadLocker::QReadLocker(QReadWriteLock *areadWriteLock)
    : q_val(reinterpret_cast<quintptr>(areadWriteLock))
{
    Q_ASSERT_X((q_val & quintptr(1u)) == quintptr(0),
               "QReadLocker", "QReadWriteLock pointer is misaligned");
    relock();
}

class QT6_ONLY(Q_CORE_EXPORT) QWriteLocker
{
public:
    Q_NODISCARD_CTOR
    inline QWriteLocker(QReadWriteLock *readWriteLock);

    inline ~QWriteLocker()
    { unlock(); }

    inline void unlock()
    {
        if (q_val) {
            if ((q_val & quintptr(1u)) == quintptr(1u)) {
                q_val &= ~quintptr(1u);
                readWriteLock()->unlock();
            }
        }
    }

    inline void relock()
    {
        if (q_val) {
            if ((q_val & quintptr(1u)) == quintptr(0u)) {
                readWriteLock()->lockForWrite();
                q_val |= quintptr(1u);
            }
        }
    }

    inline QReadWriteLock *readWriteLock() const
    { return reinterpret_cast<QReadWriteLock *>(q_val & ~quintptr(1u)); }


private:
    Q_DISABLE_COPY(QWriteLocker)
    quintptr q_val;
};

inline QWriteLocker::QWriteLocker(QReadWriteLock *areadWriteLock)
    : q_val(reinterpret_cast<quintptr>(areadWriteLock))
{
    Q_ASSERT_X((q_val & quintptr(1u)) == quintptr(0),
               "QWriteLocker", "QReadWriteLock pointer is misaligned");
    relock();
}

#if defined(Q_CC_MSVC)
#pragma warning( pop )
#endif

#else // QT_CONFIG(thread)

class QT6_ONLY(Q_CORE_EXPORT) QReadWriteLock
{
public:
    enum RecursionMode { NonRecursive, Recursive };
    inline explicit QReadWriteLock(RecursionMode = NonRecursive) noexcept { }
    inline ~QReadWriteLock() { }

    void lockForRead() noexcept { }
    bool tryLockForRead() noexcept { return true; }
    bool tryLockForRead(QDeadlineTimer) noexcept { return true; }
    bool tryLockForRead(int timeout) noexcept { Q_UNUSED(timeout); return true; }

    void lockForWrite() noexcept { }
    bool tryLockForWrite() noexcept { return true; }
    bool tryLockForWrite(QDeadlineTimer) noexcept { return true; }
    bool tryLockForWrite(int timeout) noexcept { Q_UNUSED(timeout); return true; }

    void unlock() noexcept { }

private:
    Q_DISABLE_COPY(QReadWriteLock)
};

class QT6_ONLY(Q_CORE_EXPORT) QReadLocker
{
public:
    Q_NODISCARD_CTOR
    inline explicit QReadLocker(QReadWriteLock *) noexcept { }
    inline ~QReadLocker() noexcept { }

    void unlock() noexcept { }
    void relock() noexcept { }
    QReadWriteLock *readWriteLock() noexcept { return nullptr; }

private:
    Q_DISABLE_COPY(QReadLocker)
};

class QT6_ONLY(Q_CORE_EXPORT) QWriteLocker
{
public:
    Q_NODISCARD_CTOR
    inline explicit QWriteLocker(QReadWriteLock *) noexcept { }
    inline ~QWriteLocker() noexcept { }

    void unlock() noexcept { }
    void relock() noexcept { }
    QReadWriteLock *readWriteLock() noexcept { return nullptr; }

private:
    Q_DISABLE_COPY(QWriteLocker)
};

#endif // QT_CONFIG(thread)

QT_END_NAMESPACE

#endif // QREADWRITELOCK_H
