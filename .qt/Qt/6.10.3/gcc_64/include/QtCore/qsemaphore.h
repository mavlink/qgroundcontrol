// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSEMAPHORE_H
#define QSEMAPHORE_H

#include <QtCore/qglobal.h>
#include <QtCore/qdeadlinetimer.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QSemaphorePrivate;
class Q_CORE_EXPORT QSemaphore
{
public:
    explicit QSemaphore(int n = 0);
    ~QSemaphore();

    void acquire(int n = 1);
    bool tryAcquire(int n = 1);
    QT_CORE_INLINE_SINCE(6, 6)
    bool tryAcquire(int n, int timeout);
    bool tryAcquire(int n, QDeadlineTimer timeout);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    template <typename Rep, typename Period>
    bool tryAcquire(int n, std::chrono::duration<Rep, Period> timeout)
    { return tryAcquire(n, QDeadlineTimer(timeout)); }
#endif

    void release(int n = 1);

    int available() const;

    // std::counting_semaphore compatibility:
    bool try_acquire() noexcept { return tryAcquire(); }
    template <typename Rep, typename Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period> &timeout)
    { return tryAcquire(1, timeout); }
    template <typename Clock, typename Duration>
    bool try_acquire_until(const std::chrono::time_point<Clock, Duration> &tp)
    {
        return try_acquire_for(tp - Clock::now());
    }
private:
    Q_DISABLE_COPY(QSemaphore)

    union {
        QSemaphorePrivate *d;
        QBasicAtomicInteger<quintptr> u;
        QBasicAtomicInteger<quint32> u32[2];
        QBasicAtomicInteger<quint64> u64;
    };
};

#if QT_CORE_INLINE_IMPL_SINCE(6, 6)
bool QSemaphore::tryAcquire(int n, int timeout)
{
    return tryAcquire(n, QDeadlineTimer(timeout));
}
#endif

class QSemaphoreReleaser
{
public:
    Q_NODISCARD_CTOR
    QSemaphoreReleaser() = default;
    Q_NODISCARD_CTOR
    explicit QSemaphoreReleaser(QSemaphore &sem, int n = 1) noexcept
        : m_sem(&sem), m_n(n) {}
    Q_NODISCARD_CTOR
    explicit QSemaphoreReleaser(QSemaphore *sem, int n = 1) noexcept
        : m_sem(sem), m_n(n) {}
    Q_NODISCARD_CTOR
    QSemaphoreReleaser(QSemaphoreReleaser &&other) noexcept
        : m_sem(other.cancel()), m_n(other.m_n) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QSemaphoreReleaser)

    ~QSemaphoreReleaser()
    {
        if (m_sem)
            m_sem->release(m_n);
    }

    void swap(QSemaphoreReleaser &other) noexcept
    {
        qt_ptr_swap(m_sem, other.m_sem);
        std::swap(m_n, other.m_n);
    }

    QSemaphore *semaphore() const noexcept
    { return m_sem; }

    QSemaphore *cancel() noexcept
    {
        return std::exchange(m_sem, nullptr);
    }

private:
    QSemaphore *m_sem = nullptr;
    int m_n = 0;
};

QT_END_NAMESPACE

#endif // QSEMAPHORE_H
