// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDEADLINETIMER_H
#define QDEADLINETIMER_H

#include <QtCore/qmetatype.h>
#include <QtCore/qnamespace.h>

#ifdef max
// un-pollute the namespace. We need std::numeric_limits::max() and std::chrono::duration::max()
#  undef max
#endif

#include <limits>

#include <chrono>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QDeadlineTimer
{
public:
    enum class ForeverConstant { Forever };
    static constexpr ForeverConstant Forever = ForeverConstant::Forever;

    constexpr QDeadlineTimer() noexcept = default;
    constexpr explicit QDeadlineTimer(Qt::TimerType type_) noexcept
        : type(type_) {}
    constexpr QDeadlineTimer(ForeverConstant, Qt::TimerType type_ = Qt::CoarseTimer) noexcept
        : t1((std::numeric_limits<qint64>::max)()), type(type_) {}
    explicit QDeadlineTimer(qint64 msecs, Qt::TimerType type = Qt::CoarseTimer) noexcept;

    void swap(QDeadlineTimer &other) noexcept
    { std::swap(t1, other.t1); std::swap(type, other.type); }

    constexpr bool isForever() const noexcept
    { return t1 == (std::numeric_limits<qint64>::max)(); }
    bool hasExpired() const noexcept;

    Qt::TimerType timerType() const noexcept
    { return Qt::TimerType(type & 0xff); }
    void setTimerType(Qt::TimerType type);

    qint64 remainingTime() const noexcept;
    qint64 remainingTimeNSecs() const noexcept;
    void setRemainingTime(qint64 msecs, Qt::TimerType type = Qt::CoarseTimer) noexcept;
    void setPreciseRemainingTime(qint64 secs, qint64 nsecs = 0,
                                 Qt::TimerType type = Qt::CoarseTimer) noexcept;

    qint64 deadline() const noexcept Q_DECL_PURE_FUNCTION;
    qint64 deadlineNSecs() const noexcept Q_DECL_PURE_FUNCTION;
    void setDeadline(qint64 msecs, Qt::TimerType timerType = Qt::CoarseTimer) noexcept;
    void setPreciseDeadline(qint64 secs, qint64 nsecs = 0,
                            Qt::TimerType type = Qt::CoarseTimer) noexcept;

    static QDeadlineTimer addNSecs(QDeadlineTimer dt, qint64 nsecs) noexcept Q_DECL_PURE_FUNCTION;
    static QDeadlineTimer current(Qt::TimerType timerType = Qt::CoarseTimer) noexcept;

    friend Q_CORE_EXPORT QDeadlineTimer operator+(QDeadlineTimer dt, qint64 msecs);
    friend QDeadlineTimer operator+(qint64 msecs, QDeadlineTimer dt)
    { return dt + msecs; }
    friend QDeadlineTimer operator-(QDeadlineTimer dt, qint64 msecs)
    { return dt + (-msecs); }
    friend qint64 operator-(QDeadlineTimer dt1, QDeadlineTimer dt2)
    { return (dt1.deadlineNSecs() - dt2.deadlineNSecs()) / (1000 * 1000); }
    QDeadlineTimer &operator+=(qint64 msecs)
    { *this = *this + msecs; return *this; }
    QDeadlineTimer &operator-=(qint64 msecs)
    { *this = *this + (-msecs); return *this; }

    template <class Clock, class Duration = typename Clock::duration>
    QDeadlineTimer(std::chrono::time_point<Clock, Duration> deadline_,
                   Qt::TimerType type_ = Qt::CoarseTimer) : t2(0)
    { setDeadline(deadline_, type_); }
    template <class Clock, class Duration = typename Clock::duration>
    QDeadlineTimer &operator=(std::chrono::time_point<Clock, Duration> deadline_)
    { setDeadline(deadline_); return *this; }

    template <class Clock, class Duration = typename Clock::duration>
    void setDeadline(std::chrono::time_point<Clock, Duration> tp,
                     Qt::TimerType type_ = Qt::CoarseTimer);

    template <class Clock, class Duration = typename Clock::duration>
    std::chrono::time_point<Clock, Duration> deadline() const;

    template <class Rep, class Period>
    QDeadlineTimer(std::chrono::duration<Rep, Period> remaining, Qt::TimerType type_ = Qt::CoarseTimer)
        : t2(0)
    { setRemainingTime(remaining, type_); }

    template <class Rep, class Period>
    QDeadlineTimer &operator=(std::chrono::duration<Rep, Period> remaining)
    { setRemainingTime(remaining); return *this; }

    template <class Rep, class Period>
    void setRemainingTime(std::chrono::duration<Rep, Period> remaining, Qt::TimerType type_ = Qt::CoarseTimer)
    {
        using namespace std::chrono;
        if (remaining == remaining.max())
            *this = QDeadlineTimer(Forever, type_);
        else
            setPreciseRemainingTime(0, ceil<nanoseconds>(remaining).count(), type_);
    }

    std::chrono::nanoseconds remainingTimeAsDuration() const noexcept
    {
        if (isForever())
            return std::chrono::nanoseconds::max();
        qint64 nsecs = rawRemainingTimeNSecs();
        if (nsecs <= 0)
            return std::chrono::nanoseconds::zero();
        return std::chrono::nanoseconds(nsecs);
    }

    template <class Rep, class Period>
    friend QDeadlineTimer operator+(QDeadlineTimer dt, std::chrono::duration<Rep, Period> value)
    { return QDeadlineTimer::addNSecs(dt, std::chrono::duration_cast<std::chrono::nanoseconds>(value).count()); }
    template <class Rep, class Period>
    friend QDeadlineTimer operator+(std::chrono::duration<Rep, Period> value, QDeadlineTimer dt)
    { return dt + value; }
    template <class Rep, class Period>
    friend QDeadlineTimer operator+=(QDeadlineTimer &dt, std::chrono::duration<Rep, Period> value)
    { return dt = dt + value; }

private:
    friend bool comparesEqual(const QDeadlineTimer &lhs,
                              const QDeadlineTimer &rhs) noexcept
    {
        return lhs.t1 == rhs.t1;
    }
    friend Qt::strong_ordering compareThreeWay(const QDeadlineTimer &lhs,
                                               const QDeadlineTimer &rhs) noexcept
    {
        return Qt::compareThreeWay(lhs.t1, rhs.t1);
    }
    Q_DECLARE_STRONGLY_ORDERED(QDeadlineTimer)

    qint64 t1 = 0;
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    unsigned t2 = 0;
#endif
    unsigned type = Qt::CoarseTimer;

    qint64 rawRemainingTimeNSecs() const noexcept;
};

template<class Clock, class Duration>
std::chrono::time_point<Clock, Duration> QDeadlineTimer::deadline() const
{
    using namespace std::chrono;
    if constexpr (std::is_same_v<Clock, steady_clock>) {
        auto val = duration_cast<Duration>(nanoseconds(deadlineNSecs()));
        return time_point<Clock, Duration>(val);
    } else {
        auto val = nanoseconds(rawRemainingTimeNSecs()) + Clock::now();
        return time_point_cast<Duration>(val);
    }
}

template<class Clock, class Duration>
void QDeadlineTimer::setDeadline(std::chrono::time_point<Clock, Duration> tp, Qt::TimerType type_)
{
    using namespace std::chrono;
    if (tp == tp.max()) {
        *this = Forever;
        type = type_;
    } else if constexpr (std::is_same_v<Clock, steady_clock>) {
        setPreciseDeadline(0,
                           duration_cast<nanoseconds>(tp.time_since_epoch()).count(),
                           type_);
    } else {
        setPreciseRemainingTime(0, duration_cast<nanoseconds>(tp - Clock::now()).count(), type_);
    }
}

Q_DECLARE_SHARED(QDeadlineTimer)

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QDeadlineTimer, Q_CORE_EXPORT)

#endif // QDEADLINETIMER_H
