// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QTIMER_P_H
#define QTIMER_P_H
//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt translation tools.  This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//
#include "qtimer.h"
#include "qchronotimer.h"

#include "qobject_p.h"
#include "qproperty_p.h"
#include "qttypetraits.h"

QT_BEGIN_NAMESPACE

class QTimerPrivate : public QObjectPrivate
{
public:
    QTimerPrivate(QTimer *qq)
        : q(qq),
          isQTimer(true)
    {}

    QTimerPrivate(std::chrono::nanoseconds nsec, QChronoTimer *qq)
        : intervalDuration(nsec),
          q(qq)
    {
        intervalDuration.notify();
    }
    ~QTimerPrivate() override;

    static constexpr int INV_TIMER = -1; // invalid timer id

    void setIntervalDuration(std::chrono::nanoseconds nsec)
    {
        if (isQTimer) {
            const auto msec = std::chrono::ceil<std::chrono::milliseconds>(nsec);
            static_cast<QTimer *>(q)->setInterval(msec);
        } else {
            static_cast<QChronoTimer *>(q)->setInterval(nsec);
        }
    }

    void setInterval(int msec)
    {
        Q_ASSERT(isQTimer);
        static_cast<QTimer *>(q)->setInterval(msec);
    }

    bool isActive() const { return id > Qt::TimerId::Invalid; }

    Qt::TimerId id = Qt::TimerId::Invalid;
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QTimerPrivate, int, inter, &QTimerPrivate::setInterval, 0)
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QTimerPrivate, std::chrono::nanoseconds, intervalDuration,
                                       &QTimerPrivate::setIntervalDuration,
                                       std::chrono::nanoseconds{0})
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QTimerPrivate, bool, single, false)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QTimerPrivate, Qt::TimerType, type, Qt::CoarseTimer)
    Q_OBJECT_COMPUTED_PROPERTY(QTimerPrivate, bool, isActiveData, &QTimerPrivate::isActive)

    QObject *q;
    const bool isQTimer = false; // true if q is a QTimer*
};

QT_END_NAMESPACE
#endif // QTIMER_P_H
