// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSMOOTHEDANIMATION_H
#define QQUICKSMOOTHEDANIMATION_H

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

#include <qqml.h>
#include "qquickanimation_p.h"

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQmlProperty;
class QQuickSmoothedAnimationPrivate;
class Q_QUICK_EXPORT QQuickSmoothedAnimation : public QQuickNumberAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickSmoothedAnimation)

    Q_PROPERTY(qreal velocity READ velocity WRITE setVelocity NOTIFY velocityChanged)
    Q_PROPERTY(ReversingMode reversingMode READ reversingMode WRITE setReversingMode NOTIFY reversingModeChanged)
    Q_PROPERTY(qreal maximumEasingTime READ maximumEasingTime WRITE setMaximumEasingTime NOTIFY maximumEasingTimeChanged)
    QML_NAMED_ELEMENT(SmoothedAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum ReversingMode { Eased, Immediate, Sync };
    Q_ENUM(ReversingMode)

    QQuickSmoothedAnimation(QObject *parent = nullptr);

    ReversingMode reversingMode() const;
    void setReversingMode(ReversingMode);

    int duration() const override;
    void setDuration(int) override;

    qreal velocity() const;
    void setVelocity(qreal);

    int maximumEasingTime() const;
    void setMaximumEasingTime(int);

    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
Q_SIGNALS:
    void velocityChanged();
    void reversingModeChanged();
    void maximumEasingTimeChanged();
};

QT_END_NAMESPACE

#endif // QQUICKSMOOTHEDANIMATION_H
