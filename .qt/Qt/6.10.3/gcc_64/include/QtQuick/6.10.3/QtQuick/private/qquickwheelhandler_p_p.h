// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKWHEELHANDLER_P_P_H
#define QQUICKWHEELHANDLER_P_P_H

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

#include "qquicksinglepointhandler_p_p.h"
#include "qquickwheelhandler_p.h"
#include <QtCore/qbasictimer.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickWheelHandlerPrivate : public QQuickSinglePointHandlerPrivate
{
    Q_DECLARE_PUBLIC(QQuickWheelHandler)

public:
    static QQuickWheelHandlerPrivate* get(QQuickWheelHandler *q) { return q->d_func(); }
    static const QQuickWheelHandlerPrivate* get(const QQuickWheelHandler *q) { return q->d_func(); }

    QQuickWheelHandlerPrivate();

    QMetaProperty &targetMetaProperty() const;

    QBasicTimer deactivationTimer;
    qreal activeTimeout = 0.1;
    qreal rotationScale = 1;
    qreal rotation = 0; // in units of degrees
    qreal targetScaleMultiplier = 1.25992104989487; // qPow(2, 1/3)
    QString propertyName;
    mutable QMetaProperty metaProperty;
    Qt::Orientation orientation = Qt::Vertical;
    mutable bool metaPropertyDirty = true;
    bool invertible = true;
    bool targetTransformAroundCursor = true;
    bool blocking = true;
    QQuickWheelEvent wheelEvent;
};

QT_END_NAMESPACE

#endif // QQUICKWHEELHANDLER_P_P_H
