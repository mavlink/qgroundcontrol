// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBASICTIMER_H
#define QBASICTIMER_H

#include <QtCore/qglobal.h>
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qnamespace.h>

#include <chrono>

QT_BEGIN_NAMESPACE


class QObject;

class Q_CORE_EXPORT QBasicTimer
{
    Qt::TimerId m_id;
    Q_DISABLE_COPY(QBasicTimer)

public:
    // use the same duration type
    using Duration = QAbstractEventDispatcher::Duration;

    constexpr QBasicTimer() noexcept : m_id{Qt::TimerId::Invalid} {}
    ~QBasicTimer() { if (isActive()) stop(); }

    QBasicTimer(QBasicTimer &&other) noexcept
        : m_id{std::exchange(other.m_id, Qt::TimerId::Invalid)}
    {}

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QBasicTimer)

    void swap(QBasicTimer &other) noexcept { std::swap(m_id, other.m_id); }

    bool isActive() const noexcept { return m_id != Qt::TimerId::Invalid; }
    int timerId() const noexcept { return qToUnderlying(id()); }
    Qt::TimerId id() const noexcept { return m_id; }
    QT_CORE_INLINE_SINCE(6, 5)
    void start(int msec, QObject *obj);
    QT_CORE_INLINE_SINCE(6, 5)
    void start(int msec, Qt::TimerType timerType, QObject *obj);

#if QT_CORE_REMOVED_SINCE(6, 9)
    void start(std::chrono::milliseconds duration, QObject *obj);
    void start(std::chrono::milliseconds duration, Qt::TimerType timerType, QObject *obj);
#endif
    void start(Duration duration, QObject *obj)
    { start(duration, Qt::CoarseTimer, obj); }
    void start(Duration duration, Qt::TimerType timerType, QObject *obj);
    void stop();
};
Q_DECLARE_TYPEINFO(QBasicTimer, Q_RELOCATABLE_TYPE);

#if QT_CORE_INLINE_IMPL_SINCE(6, 5)
void QBasicTimer::start(int msec, QObject *obj)
{
    start(std::chrono::milliseconds{msec}, obj);
}

void QBasicTimer::start(int msec, Qt::TimerType t, QObject *obj)
{
    start(std::chrono::milliseconds{msec}, t, obj);
}
#endif

inline void swap(QBasicTimer &lhs, QBasicTimer &rhs) noexcept { lhs.swap(rhs); }

QT_END_NAMESPACE

#endif // QBASICTIMER_H
