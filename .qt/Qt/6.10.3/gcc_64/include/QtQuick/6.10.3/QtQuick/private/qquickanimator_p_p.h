// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATOR_P_P_H
#define QQUICKANIMATOR_P_P_H

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

#include "qquickanimator_p.h"
#include "qquickanimation_p_p.h"
#include <QtQuick/qquickitem.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickAnimatorJob;

class QQuickAnimatorPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickAnimator)
public:
    QQuickAnimatorPrivate()
        : target(0)
        , duration(250)
        , from(0)
        , to(0)
        , fromIsDefined(false)
        , toIsDefined(false)
    {
    }

    QPointer<QQuickItem> target;
    int duration;
    QEasingCurve easing;
    qreal from;
    qreal to;

    uint fromIsDefined : 1;
    uint toIsDefined : 1;

    void apply(QQuickAnimatorJob *job, const QString &propertyName, QQuickStateActions &actions, QQmlProperties &modified, QObject *defaultTarget);
};

class QQuickRotationAnimatorPrivate : public QQuickAnimatorPrivate
{
public:
    QQuickRotationAnimatorPrivate()
        : direction(QQuickRotationAnimator::Numerical)
    {
    }
    QQuickRotationAnimator::RotationDirection direction;
};

#if QT_CONFIG(quick_shadereffect)
class QQuickUniformAnimatorPrivate : public QQuickAnimatorPrivate
{
public:
    QString uniform;
};
#endif

QT_END_NAMESPACE

#endif // QQUICKANIMATOR_P_P_H
