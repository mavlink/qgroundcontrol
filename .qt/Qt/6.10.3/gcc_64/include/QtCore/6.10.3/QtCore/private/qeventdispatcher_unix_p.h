// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVENTDISPATCHER_UNIX_P_H
#define QEVENTDISPATCHER_UNIX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qabstracteventdispatcher.h"
#include "QtCore/qlist.h"
#include "private/qabstracteventdispatcher_p.h"
#include "private/qcore_unix_p.h"
#include "QtCore/qvarlengtharray.h"
#include "QtCore/qhash.h"
#include "private/qtimerinfo_unix_p.h"

QT_BEGIN_NAMESPACE

class QEventDispatcherUNIXPrivate;

struct Q_CORE_EXPORT QSocketNotifierSetUNIX final
{
    inline QSocketNotifierSetUNIX() noexcept;

    inline bool isEmpty() const noexcept;
    inline short events() const noexcept;

    QSocketNotifier *notifiers[3];
};

Q_DECLARE_TYPEINFO(QSocketNotifierSetUNIX, Q_PRIMITIVE_TYPE);

struct QThreadPipe
{
    QThreadPipe();
    ~QThreadPipe();

    bool init();
    pollfd prepare() const;

    void wakeUp();
    int check(const pollfd &pfd);

    // note for eventfd(7) support:
    // fds[0] stores the eventfd, fds[1] is unused
    int fds[2] = { -1, -1 };
    QAtomicInt wakeUps;

#if defined(Q_OS_VXWORKS)
    static constexpr int len_name = 20;
    char name[len_name] = {};
#endif
};

class Q_CORE_EXPORT QEventDispatcherUNIX : public QAbstractEventDispatcherV2
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherUNIX)

public:
    explicit QEventDispatcherUNIX(QObject *parent = nullptr);
    ~QEventDispatcherUNIX();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerSocketNotifier(QSocketNotifier *notifier) final;
    void unregisterSocketNotifier(QSocketNotifier *notifier) final;

    void registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType,
                       QObject *object) override final;
    bool unregisterTimer(Qt::TimerId timerId) override final;
    bool unregisterTimers(QObject *object) override final;
    QList<TimerInfoV2> timersForObject(QObject *object) const override final;
    Duration remainingTime(Qt::TimerId timerId) const override final;

    void wakeUp() override;
    void interrupt() final;

protected:
    QEventDispatcherUNIX(QEventDispatcherUNIXPrivate &dd, QObject *parent = nullptr);
};

class Q_CORE_EXPORT QEventDispatcherUNIXPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherUNIX)

public:
    QEventDispatcherUNIXPrivate();
    ~QEventDispatcherUNIXPrivate();

    int activateTimers();

    void markPendingSocketNotifiers();
    int activateSocketNotifiers();
    void setSocketNotifierPending(QSocketNotifier *notifier);

    QThreadPipe threadPipe;
    QList<pollfd> pollfds;

    QHash<int, QSocketNotifierSetUNIX> socketNotifiers;
    QList<QSocketNotifier *> pendingNotifiers;

    QTimerInfoList timerList;
    QAtomicInt interrupt; // bool
};

inline QSocketNotifierSetUNIX::QSocketNotifierSetUNIX() noexcept
{
    notifiers[0] = nullptr;
    notifiers[1] = nullptr;
    notifiers[2] = nullptr;
}

inline bool QSocketNotifierSetUNIX::isEmpty() const noexcept
{
    return !notifiers[0] && !notifiers[1] && !notifiers[2];
}

inline short QSocketNotifierSetUNIX::events() const noexcept
{
    short result = 0;

    if (notifiers[0])
        result |= POLLIN;

    if (notifiers[1])
        result |= POLLOUT;

    if (notifiers[2])
        result |= POLLPRI;

    return result;
}

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_UNIX_P_H
