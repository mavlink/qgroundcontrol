// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTHREADPOOL_H
#define QTHREADPOOL_H

#include <QtCore/qglobal.h>

#include <QtCore/qthread.h>
#include <QtCore/qrunnable.h>

#if QT_CORE_REMOVED_SINCE(6, 6)
#include <functional>
#endif

QT_BEGIN_NAMESPACE

class QThreadPoolPrivate;
class Q_CORE_EXPORT QThreadPool : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QThreadPool)
    Q_PROPERTY(int expiryTimeout READ expiryTimeout WRITE setExpiryTimeout)
    Q_PROPERTY(int maxThreadCount READ maxThreadCount WRITE setMaxThreadCount)
    Q_PROPERTY(int activeThreadCount READ activeThreadCount)
    Q_PROPERTY(uint stackSize READ stackSize WRITE setStackSize)
    Q_PROPERTY(QThread::Priority threadPriority READ threadPriority WRITE setThreadPriority)
    friend class QFutureInterfaceBase;

public:
    QThreadPool(QObject *parent = nullptr);
    ~QThreadPool();

    static QThreadPool *globalInstance();

    void start(QRunnable *runnable, int priority = 0);
    bool tryStart(QRunnable *runnable);

#if QT_CORE_REMOVED_SINCE(6, 6)
    void start(std::function<void()> functionToRun, int priority = 0);
    bool tryStart(std::function<void()> functionToRun);
#endif

    void startOnReservedThread(QRunnable *runnable);
#if QT_CORE_REMOVED_SINCE(6, 6)
    void startOnReservedThread(std::function<void()> functionToRun);
#endif

    template <typename Callable, QRunnable::if_callable<Callable> = true>
    void start(Callable &&functionToRun, int priority = 0);
    template <typename Callable, QRunnable::if_callable<Callable> = true>
    bool tryStart(Callable &&functionToRun);
    template <typename Callable, QRunnable::if_callable<Callable> = true>
    void startOnReservedThread(Callable &&functionToRun);

    int expiryTimeout() const;
    void setExpiryTimeout(int expiryTimeout);

    int maxThreadCount() const;
    void setMaxThreadCount(int maxThreadCount);

    int activeThreadCount() const;

    void setStackSize(uint stackSize);
    uint stackSize() const;

    void setThreadPriority(QThread::Priority priority);
    QThread::Priority threadPriority() const;

    void reserveThread();
    void releaseThread();

    void setServiceLevel(QThread::QualityOfService serviceLevel);
    QThread::QualityOfService serviceLevel() const;

    QT_CORE_INLINE_SINCE(6, 8)
    bool waitForDone(int msecs);
    bool waitForDone(QDeadlineTimer deadline = QDeadlineTimer::Forever);

    void clear();

    bool contains(const QThread *thread) const;

    [[nodiscard]] bool tryTake(QRunnable *runnable);
};

template <typename Callable, QRunnable::if_callable<Callable>>
void QThreadPool::start(Callable &&functionToRun, int priority)
{
    start(QRunnable::create(std::forward<Callable>(functionToRun)), priority);
}

template <typename Callable, QRunnable::if_callable<Callable>>
bool QThreadPool::tryStart(Callable &&functionToRun)
{
    QRunnable *runnable = QRunnable::create(std::forward<Callable>(functionToRun));
    if (tryStart(runnable))
        return true;
    delete runnable;
    return false;
}

template <typename Callable, QRunnable::if_callable<Callable>>
void QThreadPool::startOnReservedThread(Callable &&functionToRun)
{
    startOnReservedThread(QRunnable::create(std::forward<Callable>(functionToRun)));
}

#if QT_CORE_INLINE_IMPL_SINCE(6, 8)
bool QThreadPool::waitForDone(int msecs)
{
    return waitForDone(QDeadlineTimer(msecs));
}
#endif

QT_END_NAMESPACE

#endif
