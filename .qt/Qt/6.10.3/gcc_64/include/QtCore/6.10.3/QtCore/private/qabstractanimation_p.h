// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTANIMATION_P_H
#define QABSTRACTANIMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.
// This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbasictimer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qelapsedtimer.h>
#include <private/qobject_p.h>
#include <private/qproperty_p.h>
#include <qabstractanimation.h>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QAnimationGroup;
class QAbstractAnimation;
class Q_CORE_EXPORT QAbstractAnimationPrivate : public QObjectPrivate
{
public:
    QAbstractAnimationPrivate();
    virtual ~QAbstractAnimationPrivate();

    static QAbstractAnimationPrivate *get(QAbstractAnimation *q)
    {
        return q->d_func();
    }

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QAbstractAnimationPrivate, QAbstractAnimation::State,
                                         state, QAbstractAnimation::Stopped)
    void setState(QAbstractAnimation::State state);

    void setDirection(QAbstractAnimation::Direction direction)
    {
        q_func()->setDirection(direction);
    }
    void emitDirectionChanged() { Q_EMIT q_func()->directionChanged(direction); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QAbstractAnimationPrivate, QAbstractAnimation::Direction,
                                       direction, &QAbstractAnimationPrivate::setDirection,
                                       &QAbstractAnimationPrivate::emitDirectionChanged,
                                       QAbstractAnimation::Forward)

    void setCurrentTime(int msecs) { q_func()->setCurrentTime(msecs); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QAbstractAnimationPrivate, int, totalCurrentTime,
                                       &QAbstractAnimationPrivate::setCurrentTime, 0)
    int currentTime = 0;

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QAbstractAnimationPrivate, int, loopCount, 1)

    void emitCurrentLoopChanged() { Q_EMIT q_func()->currentLoopChanged(currentLoop); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QAbstractAnimationPrivate, int, currentLoop, nullptr,
                                       &QAbstractAnimationPrivate::emitCurrentLoopChanged, 0)

    bool deleteWhenStopped = false;
    bool hasRegisteredTimer = false;
    bool isPause = false;
    bool isGroup = false;

    QAnimationGroup *group = nullptr;

private:
    Q_DECLARE_PUBLIC(QAbstractAnimation)
};


class QUnifiedTimer;
class QDefaultAnimationDriver : public QAnimationDriver
{
    Q_OBJECT
public:
    explicit QDefaultAnimationDriver(QUnifiedTimer *timer);
    ~QDefaultAnimationDriver() override;

protected:
    void timerEvent(QTimerEvent *e) override;

private Q_SLOTS:
    void startTimer();
    void stopTimer();

private:
    QBasicTimer m_timer;
    QUnifiedTimer *m_unified_timer;
};

class Q_CORE_EXPORT QAnimationDriverPrivate : public QObjectPrivate
{
public:
    QAnimationDriverPrivate();
    ~QAnimationDriverPrivate() override;

    QElapsedTimer timer;
    bool running = false;
};

class Q_CORE_EXPORT QAbstractAnimationTimer : public QObject
{
    Q_OBJECT
public:
    QAbstractAnimationTimer();
    ~QAbstractAnimationTimer() override;

    virtual void updateAnimationsTime(qint64 delta) = 0;
    virtual void restartAnimationTimer() = 0;
#define QT_QAbstractAnimationTimer_runningAnimationCount_IS_CONST
    virtual qsizetype runningAnimationCount() const = 0;

    bool isRegistered = false;
    bool isPaused = false;
    int pauseDuration = 0;
};

class Q_CORE_EXPORT QUnifiedTimer : public QObject
{
    Q_OBJECT
private:
    QUnifiedTimer();

public:
    ~QUnifiedTimer() override;

    static QUnifiedTimer *instance();
    static QUnifiedTimer *instance(bool create);

    static void startAnimationTimer(QAbstractAnimationTimer *timer);
    static void stopAnimationTimer(QAbstractAnimationTimer *timer);

    static void pauseAnimationTimer(QAbstractAnimationTimer *timer, int duration);
    static void resumeAnimationTimer(QAbstractAnimationTimer *timer);

    //defines the timing interval. Default is DEFAULT_TIMER_INTERVAL
    void setTimingInterval(int interval);

    /*
       this allows to have a consistent timer interval at each tick from the timer
       not taking the real time that passed into account.
    */
    void setConsistentTiming(bool consistent) { consistentTiming = consistent; }

    //these facilitate fine-tuning of complex animations
    void setSlowModeEnabled(bool enabled) { slowMode = enabled; }
    void setSlowdownFactor(qreal factor) { slowdownFactor = factor; }

    void installAnimationDriver(QAnimationDriver *driver);
    void uninstallAnimationDriver(QAnimationDriver *driver);
    bool canUninstallAnimationDriver(QAnimationDriver *driver);

    void restart();
    void maybeUpdateAnimationsToCurrentTime();
    void updateAnimationTimers();

    //useful for profiling/debugging
    qsizetype runningAnimationCount() const;
    void registerProfilerCallback(void (*cb)(qint64));

    void startAnimationDriver();
    void stopAnimationDriver();
    qint64 elapsed() const;

protected:
    void timerEvent(QTimerEvent *) override;

private Q_SLOTS:
    void startTimers();
    void stopTimer();

private:
    friend class QDefaultAnimationDriver;
    friend class QAnimationDriver;

    QAnimationDriver *driver;
    QDefaultAnimationDriver defaultDriver;

    QBasicTimer pauseTimer;

    QElapsedTimer time;

    qint64 lastTick;
    int timingInterval;
    int currentAnimationIdx;
    bool insideTick;
    bool insideRestart;
    bool consistentTiming;
    bool slowMode;
    bool startTimersPending;
    bool stopTimerPending;
    bool allowNegativeDelta;

    // This factor will be used to divide the DEFAULT_TIMER_INTERVAL at each tick
    // when slowMode is enabled. Setting it to 0 or higher than DEFAULT_TIMER_INTERVAL (16)
    // stops all animations.
    qreal slowdownFactor;

    QList<QAbstractAnimationTimer*> animationTimers, animationTimersToStart;
    QList<QAbstractAnimationTimer*> pausedAnimationTimers;

    void localRestart();
    int closestPausedAnimationTimerTimeToFinish();

    void (*profilerCallback)(qint64);

    qint64 driverStartTime; // The time the animation driver was started
    qint64 temporalDrift; // The delta between animation driver time and wall time.
};

class QAnimationTimer : public QAbstractAnimationTimer
{
    Q_OBJECT
private:
    QAnimationTimer();

public:
    ~QAnimationTimer() override;

    static QAnimationTimer *instance();
    static QAnimationTimer *instance(bool create);

    static void registerAnimation(QAbstractAnimation *animation, bool isTopLevel);
    static void unregisterAnimation(QAbstractAnimation *animation);

    /*
        this is used for updating the currentTime of all animations in case the pause
        timer is active or, otherwise, only of the animation passed as parameter.
    */
    static void ensureTimerUpdate();

    /*
        this will evaluate the need of restarting the pause timer in case there is still
        some pause animations running.
    */
    static void updateAnimationTimer();

    void restartAnimationTimer() override;
    void updateAnimationsTime(qint64 delta) override;

    //useful for profiling/debugging
    qsizetype runningAnimationCount() const override { return animations.size(); }

private Q_SLOTS:
    void startAnimations();
    void stopTimer();

private:
    qint64 lastTick;
    int currentAnimationIdx;
    bool insideTick;
    bool startAnimationPending;
    bool stopTimerPending;

    QList<QAbstractAnimation*> animations, animationsToStart;

    // this is the count of running animations that are not a group neither a pause animation
    int runningLeafAnimations;
    QList<QAbstractAnimation*> runningPauseAnimations;

    void registerRunningAnimation(QAbstractAnimation *animation);
    void unregisterRunningAnimation(QAbstractAnimation *animation);

    int closestPauseAnimationTimeToFinish();
};

QT_END_NAMESPACE

#endif //QABSTRACTANIMATION_P_H
