// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QABSTRACTANIMATIONJOB_P_H
#define QABSTRACTANIMATIONJOB_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qanimationjobutil_p.h>
#include <private/qdoubleendedlist_p.h>
#include <QtCore/QObject>
#include <QtCore/private/qabstractanimation_p.h>
#include <vector>

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class QAnimationGroupJob;
class QAnimationJobChangeListener;
class QQmlAnimationTimer;

class Q_QML_EXPORT QAbstractAnimationJob : public QInheritedListNode
{
    Q_DISABLE_COPY(QAbstractAnimationJob)
public:
    enum Direction {
        Forward,
        Backward
    };

    enum State {
        Stopped,
        Paused,
        Running
    };

    QAbstractAnimationJob();
    virtual ~QAbstractAnimationJob();

    //definition
    inline QAnimationGroupJob *group() const {return m_group;}

    inline int loopCount() const {return m_loopCount;}
    void setLoopCount(int loopCount);

    int totalDuration() const;
    virtual int duration() const {return 0;}

    inline QAbstractAnimationJob::Direction direction() const {return m_direction;}
    void setDirection(QAbstractAnimationJob::Direction direction);

    //state
    inline int currentTime() const {return m_totalCurrentTime;}
    inline int currentLoopTime() const {return m_currentTime;}
    inline int currentLoop() const {return m_currentLoop;}
    inline QAbstractAnimationJob::State state() const {return m_state;}
    inline bool isRunning() { return m_state == Running; }
    inline bool isStopped() { return m_state == Stopped; }
    inline bool isPaused() { return m_state == Paused; }
    void setDisableUserControl();
    void setEnableUserControl();
    bool userControlDisabled() const;

    void setCurrentTime(int msecs);

    void start();
    void pause();
    void resume();
    void stop();
    void complete();

    enum ChangeType {
        Completion = 0x01,
        StateChange = 0x02,
        CurrentLoop = 0x04,
        CurrentTime = 0x08
    };
    Q_DECLARE_FLAGS(ChangeTypes, ChangeType)

    void addAnimationChangeListener(QAnimationJobChangeListener *listener, QAbstractAnimationJob::ChangeTypes);
    void removeAnimationChangeListener(QAnimationJobChangeListener *listener, QAbstractAnimationJob::ChangeTypes);

    bool isGroup() const { return m_isGroup; }
    bool isRenderThreadJob() const { return m_isRenderThreadJob; }
    bool isRenderThreadProxy() const { return m_isRenderThreadProxy; }

    SelfDeletable m_selfDeletable;
protected:
    virtual void updateCurrentTime(int) {}
    virtual void updateLoopCount(int) {}
    virtual void updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState);
    virtual void updateDirection(QAbstractAnimationJob::Direction direction);
    virtual void topLevelAnimationLoopChanged() {}

    virtual void debugAnimation(QDebug d) const;

    void fireTopLevelAnimationLoopChanged();

    void setState(QAbstractAnimationJob::State state);

    void finished();
    void stateChanged(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState);
    void currentLoopChanged();
    void directionChanged(QAbstractAnimationJob::Direction);
    void currentTimeChanged(int currentTime);

    //definition
    int m_loopCount;
    QAnimationGroupJob *m_group;
    QAbstractAnimationJob::Direction m_direction;

    //state
    QAbstractAnimationJob::State m_state;
    int m_totalCurrentTime;
    int m_currentTime;
    int m_currentLoop;
    //records the finish time for an uncontrolled animation (used by animation groups)
    int m_uncontrolledFinishTime;
    int m_currentLoopStartTime; // used together with m_uncontrolledFinishTime

    struct ChangeListener {
        ChangeListener(QAnimationJobChangeListener *l, QAbstractAnimationJob::ChangeTypes t) : listener(l), types(t) {}
        QAnimationJobChangeListener *listener;
        QAbstractAnimationJob::ChangeTypes types;
        bool operator==(const ChangeListener &other) const { return listener == other.listener && types == other.types; }
    };
    std::vector<ChangeListener> changeListeners;

    QQmlAnimationTimer *m_timer = nullptr;

    bool m_hasRegisteredTimer:1;
    bool m_isPause:1;
    bool m_isGroup:1;
    bool m_disableUserControl:1;
    bool m_hasCurrentTimeChangeListeners:1;
    bool m_isRenderThreadJob:1;
    bool m_isRenderThreadProxy:1;

    friend class QQmlAnimationTimer;
    friend class QAnimationGroupJob;
    friend Q_QML_EXPORT QDebug operator<<(QDebug, const QAbstractAnimationJob *job);
};

class Q_QML_EXPORT QAnimationJobChangeListener
{
public:
    virtual ~QAnimationJobChangeListener();
    virtual void animationFinished(QAbstractAnimationJob *) {}
    virtual void animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State, QAbstractAnimationJob::State) {}
    virtual void animationCurrentLoopChanged(QAbstractAnimationJob *) {}
    virtual void animationCurrentTimeChanged(QAbstractAnimationJob *, int) {}
};

class Q_QML_EXPORT QQmlAnimationTimer : public QAbstractAnimationTimer
{
    Q_OBJECT
private:
    QQmlAnimationTimer();

public:
    ~QQmlAnimationTimer(); // must be destructible by QThreadStorage

    static QQmlAnimationTimer *instance();
    static QQmlAnimationTimer *instance(bool create);

    void registerAnimation(QAbstractAnimationJob *animation, bool isTopLevel);
    void unregisterAnimation(QAbstractAnimationJob *animation);

    /*
        this is used for updating the currentTime of all animations in case the pause
        timer is active or, otherwise, only of the animation passed as parameter.
    */
    void ensureTimerUpdate();

    /*
        this will evaluate the need of restarting the pause timer in case there is still
        some pause animations running.
    */
    void updateAnimationTimer();

    void restartAnimationTimer() override;
    void updateAnimationsTime(qint64 timeStep) override;

    //useful for profiling/debugging
#ifdef QT_QAbstractAnimationTimer_runningAnimationCount_IS_CONST
    qsizetype runningAnimationCount() const override { return animations.size(); }
#else
    int runningAnimationCount() override { return animations.size(); }
#endif

    bool hasStartAnimationPending() const { return startAnimationPending; }

public Q_SLOTS:
    void startAnimations();
    void stopTimer();

private:
    qint64 lastTick;
    int currentAnimationIdx;
    bool insideTick;
    bool startAnimationPending;
    bool stopTimerPending;

    QList<QAbstractAnimationJob*> animations, animationsToStart;

    // this is the count of running animations that are not a group neither a pause animation
    int runningLeafAnimations;
    QList<QAbstractAnimationJob*> runningPauseAnimations;

    void registerRunningAnimation(QAbstractAnimationJob *animation);
    void unregisterRunningAnimation(QAbstractAnimationJob *animation);
    void unsetJobTimer(QAbstractAnimationJob *animation);

    int closestPauseAnimationTimeToFinish();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractAnimationJob::ChangeTypes)

Q_QML_EXPORT QDebug operator<<(QDebug, const QAbstractAnimationJob *job);

QT_END_NAMESPACE

#endif // QABSTRACTANIMATIONJOB_P_H
