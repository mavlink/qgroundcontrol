// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKWHEELHANDLER_H
#define QQUICKWHEELHANDLER_H

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

#include <QtGui/qevent.h>
#include <QtQuick/qquickitem.h>

#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE

class QQuickWheelEvent;
class QQuickWheelHandlerPrivate;

class Q_QUICK_EXPORT QQuickWheelHandler : public QQuickSinglePointHandler
{
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(bool invertible READ isInvertible WRITE setInvertible NOTIFY invertibleChanged)
    Q_PROPERTY(qreal activeTimeout READ activeTimeout WRITE setActiveTimeout NOTIFY activeTimeoutChanged)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(qreal rotationScale READ rotationScale WRITE setRotationScale NOTIFY rotationScaleChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(qreal targetScaleMultiplier READ targetScaleMultiplier WRITE setTargetScaleMultiplier NOTIFY targetScaleMultiplierChanged)
    Q_PROPERTY(bool targetTransformAroundCursor READ isTargetTransformAroundCursor WRITE setTargetTransformAroundCursor NOTIFY targetTransformAroundCursorChanged)
    Q_PROPERTY(bool blocking READ isBlocking WRITE setBlocking NOTIFY blockingChanged REVISION(6, 3))

    QML_NAMED_ELEMENT(WheelHandler)
    QML_ADDED_IN_VERSION(2, 14)

public:
    explicit QQuickWheelHandler(QQuickItem *parent = nullptr);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    bool isInvertible() const;
    void setInvertible(bool invertible);

    qreal activeTimeout() const;
    void setActiveTimeout(qreal timeout);

    qreal rotation() const;
    void setRotation(qreal rotation);

    qreal rotationScale() const;
    void setRotationScale(qreal rotationScale);

    QString property() const;
    void setProperty(const QString &name);

    qreal targetScaleMultiplier() const;
    void setTargetScaleMultiplier(qreal targetScaleMultiplier);

    bool isTargetTransformAroundCursor() const;
    void setTargetTransformAroundCursor(bool ttac);

    bool isBlocking() const;
    void setBlocking(bool blocking);

Q_SIGNALS:
    void wheel(QQuickWheelEvent *event);

    void orientationChanged();
    void invertibleChanged();
    void activeTimeoutChanged();
    void rotationChanged();
    void rotationScaleChanged();
    void propertyChanged();
    void targetScaleMultiplierChanged();
    void targetTransformAroundCursorChanged();
    Q_REVISION(6, 3) void blockingChanged();

protected:
    bool wantsPointerEvent(QPointerEvent *event) override;
    void handleEventPoint(QPointerEvent *event, QEventPoint &point) override;
    void onTargetChanged(QQuickItem *oldTarget) override;
    void onActiveChanged() override;
    void timerEvent(QTimerEvent *event) override;

    Q_DECLARE_PRIVATE(QQuickWheelHandler)
};

QT_END_NAMESPACE

#endif // QQUICKWHEELHANDLER_H
