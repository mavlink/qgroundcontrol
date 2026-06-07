// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QPARALLELANIMATIONGROUPJOB_P_H
#define QPARALLELANIMATIONGROUPJOB_P_H

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

#include "private/qanimationgroupjob_p.h"

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QParallelAnimationGroupJob : public QAnimationGroupJob
{
    Q_DISABLE_COPY(QParallelAnimationGroupJob)
public:
    QParallelAnimationGroupJob();
    ~QParallelAnimationGroupJob();

    int duration() const override;

protected:
    void updateCurrentTime(int currentTime) override;
    void updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState) override;
    void updateDirection(QAbstractAnimationJob::Direction direction) override;
    void uncontrolledAnimationFinished(QAbstractAnimationJob *animation) override;
    void debugAnimation(QDebug d) const override;

private:
    bool shouldAnimationStart(QAbstractAnimationJob *animation, bool startIfAtEnd) const;
    void applyGroupState(QAbstractAnimationJob *animation);

    //state
    int m_previousLoop = 0;
    int m_previousCurrentTime = 0;
};

QT_END_NAMESPACE

#endif // QPARALLELANIMATIONGROUPJOB_P_H
