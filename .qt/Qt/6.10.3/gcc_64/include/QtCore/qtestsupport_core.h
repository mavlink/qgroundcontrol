// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTSUPPORT_CORE_H
#define QTESTSUPPORT_CORE_H

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>

#include <chrono>

QT_BEGIN_NAMESPACE

namespace QTest {

Q_CORE_EXPORT void qSleep(int ms);
Q_CORE_EXPORT void qSleep(std::chrono::milliseconds msecs);

namespace Internal
{
static inline constexpr std::chrono::milliseconds defaultTryTimeout
    = std::chrono::milliseconds(5000);
} // namespace Internal

template <typename Functor>
[[nodiscard]] bool
qWaitFor(Functor predicate, QDeadlineTimer deadline = QDeadlineTimer(Internal::defaultTryTimeout))
{
    // We should not spin the event loop in case the predicate is already true,
    // otherwise we might send new events that invalidate the predicate.
    if (predicate())
        return true;

    // qWait() is expected to spin the event loop at least once, even when
    // called with a small timeout like 1ns.

    do {
        // We explicitly do not pass the remaining time to processEvents, as
        // that would keep spinning processEvents for the whole duration if
        // new events were posted as part of processing events, and we need
        // to return back to this function to check the predicate between
        // each pass of processEvents. Our own timer will take care of the
        // timeout.
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        if (predicate())
            return true;

        using namespace std::chrono;

        if (const auto remaining = deadline.remainingTimeAsDuration(); remaining > 0ns)
            qSleep((std::min)(10ms, ceil<milliseconds>(remaining)));

    } while (!deadline.hasExpired());

    return predicate(); // Last chance
}

template <typename Functor>
[[nodiscard]] bool qWaitFor(Functor predicate, int timeout)
{
    return qWaitFor(predicate, QDeadlineTimer{timeout, Qt::PreciseTimer});
}

Q_CORE_EXPORT void qWait(int ms);

Q_CORE_EXPORT void qWait(std::chrono::milliseconds msecs);

} // namespace QTest

QT_END_NAMESPACE

#endif
