// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QSEQUENTIALANIMATIONGROUPJOB_P_H
#define QSEQUENTIALANIMATIONGROUPJOB_P_H

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

#include <private/qanimationgroupjob_p.h>

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class QPauseAnimationJob;
class Q_QML_EXPORT QSequentialAnimationGroupJob : public QAnimationGroupJob
{
    Q_DISABLE_COPY(QSequentialAnimationGroupJob)
public:
    QSequentialAnimationGroupJob();
    ~QSequentialAnimationGroupJob();

    int duration() const override;

    QAbstractAnimationJob *currentAnimation() const { return m_currentAnimation; }
    void clear() override;

protected:
    void updateCurrentTime(int) override;
    void updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState) override;
    void updateDirection(QAbstractAnimationJob::Direction direction) override;
    void uncontrolledAnimationFinished(QAbstractAnimationJob *animation) override;
    void debugAnimation(QDebug d) const override;

private:
    struct AnimationIndex
    {
        AnimationIndex() {}
        // AnimationIndex points to the animation at timeOffset, skipping 0 duration animations.
        // Note that the index semantic is slightly different depending on the direction.
        bool afterCurrent = false;  //whether animation is before or after m_currentAnimation   //TODO: make enum Before/After/Same
        int timeOffset = 0; // time offset when the animation at index starts.
        const QAbstractAnimationJob *animation = nullptr; //points to the animation at timeOffset
    };

    int animationActualTotalDuration(const QAbstractAnimationJob *anim) const;
    AnimationIndex indexForCurrentTime() const;

    void setCurrentAnimation(const QAbstractAnimationJob *anim, bool intermediate = false);
    void activateCurrentAnimation(bool intermediate = false);

    void animationInserted(QAbstractAnimationJob *anim) override;
    void animationRemoved(QAbstractAnimationJob *anim, QAbstractAnimationJob *, QAbstractAnimationJob *) override;

    bool atEnd() const;

    void restart();

    // handle time changes
    void rewindForwards(const AnimationIndex &newAnimationIndex);
    void advanceForwards(const AnimationIndex &newAnimationIndex);

    //state
    QAbstractAnimationJob *m_currentAnimation = nullptr;
    int m_previousLoop = 0;
};

QT_END_NAMESPACE

#endif //QSEQUENTIALANIMATIONGROUPJOB_P_H
