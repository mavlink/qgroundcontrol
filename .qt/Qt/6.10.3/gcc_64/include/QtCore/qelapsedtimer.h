// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QELAPSEDTIMER_H
#define QELAPSEDTIMER_H

#include <QtCore/qcompare.h>
#include <QtCore/qglobal.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QElapsedTimer
{
public:
    enum ClockType {
        SystemTime,
        MonotonicClock,
        TickCounter Q_DECL_ENUMERATOR_DEPRECATED_X(
                "Not supported anymore. Use PerformanceCounter instead."),
        MachAbsoluteTime,
        PerformanceCounter
    };

    // similar to std::chrono::*_clock
    using Duration = std::chrono::nanoseconds;
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock, Duration>;

    constexpr QElapsedTimer() = default;

    static ClockType clockType() noexcept;
    static bool isMonotonic() noexcept;

    void start() noexcept;
    qint64 restart() noexcept;
    void invalidate() noexcept;
    bool isValid() const noexcept;

    Duration durationElapsed() const noexcept;
    qint64 nsecsElapsed() const noexcept;
    qint64 elapsed() const noexcept;
    bool hasExpired(qint64 timeout) const noexcept;

    qint64 msecsSinceReference() const noexcept;
    Duration durationTo(const QElapsedTimer &other) const noexcept;
    qint64 msecsTo(const QElapsedTimer &other) const noexcept;
    qint64 secsTo(const QElapsedTimer &other) const noexcept;
    friend bool Q_CORE_EXPORT operator<(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept;

private:
    friend bool comparesEqual(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept
    {
        return lhs.t1 == rhs.t1 && lhs.t2 == rhs.t2;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QElapsedTimer)

    friend Qt::strong_ordering compareThreeWay(const QElapsedTimer &lhs,
                                               const QElapsedTimer &rhs) noexcept
    {
        return Qt::compareThreeWay(lhs.t1, rhs.t1);
    }

#if defined(__cpp_lib_three_way_comparison)
    friend std::strong_ordering
    operator<=>(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept
    {
        return compareThreeWay(lhs, rhs);
    }
#else
    friend bool operator>(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept
    {
        return is_gt(compareThreeWay(lhs, rhs));
    }
    friend bool operator<=(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept
    {
        return is_lteq(compareThreeWay(lhs, rhs));
    }
    friend bool operator>=(const QElapsedTimer &lhs, const QElapsedTimer &rhs) noexcept
    {
        return is_gteq(compareThreeWay(lhs, rhs));
    }
#endif // defined(__cpp_lib_three_way_comparison)
    qint64 t1 = Q_INT64_C(0x8000000000000000);
    qint64 t2 = Q_INT64_C(0x8000000000000000);
};

QT_END_NAMESPACE

#endif // QELAPSEDTIMER_H
