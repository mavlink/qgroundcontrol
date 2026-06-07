// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPARALLELANIMATIONGROUP_P_H
#define QPARALLELANIMATIONGROUP_P_H

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

#include "qparallelanimationgroup.h"
#include "private/qanimationgroup_p.h"
#include <QtCore/qhash.h>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QParallelAnimationGroupPrivate : public QAnimationGroupPrivate
{
    Q_DECLARE_PUBLIC(QParallelAnimationGroup)
public:
    QParallelAnimationGroupPrivate()
        : lastLoop(0), lastCurrentTime(0)
    {
    }

    QHash<QAbstractAnimation*, int> uncontrolledFinishTime;
    int lastLoop;
    int lastCurrentTime;

    bool shouldAnimationStart(QAbstractAnimation *animation, bool startIfAtEnd) const;
    void applyGroupState(QAbstractAnimation *animation);
    bool isUncontrolledAnimationFinished(QAbstractAnimation *anim) const;
    void connectUncontrolledAnimations();
    void disconnectUncontrolledAnimations();

    void animationRemoved(qsizetype index, QAbstractAnimation *) override;

    // private slot
    void _q_uncontrolledAnimationFinished();
};

QT_END_NAMESPACE

#endif //QPARALLELANIMATIONGROUP_P_H
