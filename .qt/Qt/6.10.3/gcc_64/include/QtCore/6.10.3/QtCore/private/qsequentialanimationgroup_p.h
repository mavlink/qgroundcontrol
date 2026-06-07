// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSEQUENTIALANIMATIONGROUP_P_H
#define QSEQUENTIALANIMATIONGROUP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsequentialanimationgroup.h"
#include "private/qanimationgroup_p.h"
#include "private/qproperty_p.h"

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QSequentialAnimationGroupPrivate : public QAnimationGroupPrivate
{
    Q_DECLARE_PUBLIC(QSequentialAnimationGroup)
public:
    QSequentialAnimationGroupPrivate() : currentAnimationIndex(-1), lastLoop(0) { }

    struct AnimationIndex
    {
        AnimationIndex() : index(0), timeOffset(0) {}
        // index points to the animation at timeOffset, skipping 0 duration animations.
        // Note that the index semantic is slightly different depending on the direction.
        int index; // the index of the animation in timeOffset
        int timeOffset; // time offset when the animation at index starts.
    };

    int animationActualTotalDuration(int index) const;
    AnimationIndex indexForCurrentTime() const;

    void setCurrentAnimation(int index, bool intermediate = false);
    void activateCurrentAnimation(bool intermediate = false);

    void animationInsertedAt(qsizetype index) override;
    void animationRemoved(qsizetype index, QAbstractAnimation *anim) override;

    bool atEnd() const;

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QSequentialAnimationGroupPrivate, QAbstractAnimation *,
                                         currentAnimation,
                                         nullptr // initial value
    )
    int currentAnimationIndex;

    // this is the actual duration of uncontrolled animations
    // it helps seeking and even going forward
    QList<int> actualDuration;

    void restart();
    int lastLoop;

    // handle time changes
    void rewindForwards(const AnimationIndex &newAnimationIndex);
    void advanceForwards(const AnimationIndex &newAnimationIndex);

    // private slot
    void _q_uncontrolledAnimationFinished();
};

QT_END_NAMESPACE

#endif //QSEQUENTIALANIMATIONGROUP_P_H
