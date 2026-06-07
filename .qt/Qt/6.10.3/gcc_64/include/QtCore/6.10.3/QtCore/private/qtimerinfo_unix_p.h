// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTIMERINFO_UNIX_P_H
#define QTIMERINFO_UNIX_P_H

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

#include <QtCore/private/qglobal_p.h>

#include "qabstracteventdispatcher.h"

#include <sys/time.h> // struct timespec
#include <chrono>

QT_BEGIN_NAMESPACE

// internal timer info
struct QTimerInfo
{
    using Duration = QAbstractEventDispatcher::Duration;
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock, Duration>;
    QTimerInfo(Qt::TimerId timerId, Duration interval, Qt::TimerType type, QObject *obj)
        : interval(interval), id(timerId), timerType(type), obj(obj)
    {
    }

    TimePoint timeout = {};                     // - when to actually fire
    Duration interval = Duration{-1};           // - timer interval
    Qt::TimerId id = Qt::TimerId::Invalid;      // - timer identifier
    Qt::TimerType timerType; // - timer type
    QObject *obj = nullptr; // - object to receive event
    QTimerInfo **activateRef = nullptr; // - ref from activateTimers
};

class Q_CORE_EXPORT QTimerInfoList
{
public:
    using Duration = QAbstractEventDispatcher::Duration;
    using TimerInfo = QAbstractEventDispatcher::TimerInfoV2;
    QTimerInfoList();

    mutable std::chrono::steady_clock::time_point currentTime;

    std::optional<Duration> timerWait();
    void timerInsert(QTimerInfo *);

    Duration remainingDuration(Qt::TimerId timerId) const;

    void registerTimer(Qt::TimerId timerId, Duration interval,
                       Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(Qt::TimerId timerId);
    bool unregisterTimers(QObject *object);
    QList<TimerInfo> registeredTimers(QObject *object) const;

    int activateTimers();
    bool hasPendingTimers();

    void clearTimers()
    {
        qDeleteAll(timers);
        timers.clear();
    }

    bool isEmpty() const { return timers.empty(); }

    qsizetype size() const { return timers.size(); }

    auto findTimerById(Qt::TimerId timerId) const
    {
        auto matchesId = [timerId](const auto &t) { return t->id == timerId; };
        return std::find_if(timers.cbegin(), timers.cend(), matchesId);
    }

private:
    std::chrono::steady_clock::time_point updateCurrentTime() const;

    // state variables used by activateTimers()
    QTimerInfo *firstTimerInfo = nullptr;
    QList<QTimerInfo *> timers;
};

QT_END_NAMESPACE

#endif // QTIMERINFO_UNIX_P_H
