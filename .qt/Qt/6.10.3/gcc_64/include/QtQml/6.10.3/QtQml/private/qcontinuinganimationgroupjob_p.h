// Copyright (C) 2016 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QCONTINUINGANIMATIONGROUPJOB_P_H
#define QCONTINUINGANIMATIONGROUPJOB_P_H

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

class Q_QML_EXPORT QContinuingAnimationGroupJob : public QAnimationGroupJob
{
    Q_DISABLE_COPY(QContinuingAnimationGroupJob)
public:
    QContinuingAnimationGroupJob();
    ~QContinuingAnimationGroupJob();

    int duration() const override { return -1; }

protected:
    void updateCurrentTime(int currentTime) override;
    void updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState) override;
    void updateDirection(QAbstractAnimationJob::Direction direction) override;
    void uncontrolledAnimationFinished(QAbstractAnimationJob *animation) override;
    void debugAnimation(QDebug d) const override;
};

QT_END_NAMESPACE

#endif // QCONTINUINGANIMATIONGROUPJOB_P_H
