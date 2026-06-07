// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVENTDISPATCHER_GLIB_P_H
#define QEVENTDISPATCHER_GLIB_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qabstracteventdispatcher.h"
#include "qabstracteventdispatcher_p.h"

typedef struct _GMainContext GMainContext;

QT_BEGIN_NAMESPACE

class QEventDispatcherGlibPrivate;

class Q_CORE_EXPORT QEventDispatcherGlib : public QAbstractEventDispatcherV2
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherGlib)

public:
    explicit QEventDispatcherGlib(QObject *parent = nullptr);
    explicit QEventDispatcherGlib(GMainContext *context, QObject *parent = nullptr);
    ~QEventDispatcherGlib();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerSocketNotifier(QSocketNotifier *socketNotifier) final;
    void unregisterSocketNotifier(QSocketNotifier *socketNotifier) final;

    void registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType,
                       QObject *object) override final;
    bool unregisterTimer(Qt::TimerId timerId) override final;
    bool unregisterTimers(QObject *object) override final;
    QList<TimerInfoV2> timersForObject(QObject *object) const override final;
    Duration remainingTime(Qt::TimerId timerId) const override final;

    void wakeUp() final;
    void interrupt() final;

    static bool versionSupported();

protected:
    QEventDispatcherGlib(QEventDispatcherGlibPrivate &dd, QObject *parent);
};

struct GPostEventSource;
struct GSocketNotifierSource;
struct GTimerSource;
struct GIdleTimerSource;

class Q_CORE_EXPORT QEventDispatcherGlibPrivate : public QAbstractEventDispatcherPrivate
{

public:
    QEventDispatcherGlibPrivate(GMainContext *context = nullptr);
    ~QEventDispatcherGlibPrivate() override;

    GMainContext *mainContext;
    GPostEventSource *postEventSource;
    GSocketNotifierSource *socketNotifierSource;
    GTimerSource *timerSource;
    GIdleTimerSource *idleTimerSource;
    bool wakeUpCalled = true;

    void runTimersOnceWithNormalPriority();
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_GLIB_P_H
