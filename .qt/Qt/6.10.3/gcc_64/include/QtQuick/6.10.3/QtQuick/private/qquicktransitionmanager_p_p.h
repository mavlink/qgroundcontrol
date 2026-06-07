// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTRANSITIONMANAGER_P_H
#define QQUICKTRANSITIONMANAGER_P_H

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

#include "qquickanimation_p.h"
#include <private/qanimationjobutil_p.h>

QT_BEGIN_NAMESPACE

class QQuickState;
class QQuickStateAction;
class QQuickTransitionManagerPrivate;
class Q_QUICK_EXPORT QQuickTransitionManager
{
public:
    QQuickTransitionManager();
    virtual ~QQuickTransitionManager();

    bool isRunning() const;

    void transition(const QList<QQuickStateAction> &, QQuickTransition *transition, QObject *defaultTarget = nullptr);

    void cancel();

    SelfDeletable m_selfDeletable;
protected:
    virtual void finished();

private:
    Q_DISABLE_COPY(QQuickTransitionManager)
    QQuickTransitionManagerPrivate *d;

    void complete();
    void setState(QQuickState *);

    friend class QQuickState;
    friend class ParallelAnimationWrapper;
};

QT_END_NAMESPACE

#endif // QQUICKTRANSITIONMANAGER_P_H
