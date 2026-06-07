// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOINTERSINGLEHANDLER_H
#define QQUICKPOINTERSINGLEHANDLER_H

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

#include "qquickhandlerpoint_p.h"
#include "qquickpointerdevicehandler_p.h"

QT_BEGIN_NAMESPACE

class QQuickSinglePointHandlerPrivate;

class Q_QUICK_EXPORT QQuickSinglePointHandler : public QQuickPointerDeviceHandler
{
    Q_OBJECT
    Q_PROPERTY(QQuickHandlerPoint point READ point NOTIFY pointChanged)

public:
    explicit QQuickSinglePointHandler(QQuickItem *parent = nullptr);

    QQuickHandlerPoint point() const;

Q_SIGNALS:
    void pointChanged();

protected:
    QQuickSinglePointHandler(QQuickSinglePointHandlerPrivate &dd, QQuickItem *parent);

    bool wantsPointerEvent(QPointerEvent *event) override;
    void handlePointerEventImpl(QPointerEvent *event) override;
    virtual void handleEventPoint(QPointerEvent *event, QEventPoint &point);

    QEventPoint &currentPoint(QPointerEvent *ev);
    void onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point) override;

    void setIgnoreAdditionalPoints(bool v = true);

    void moveTarget(QPointF pos, QEventPoint &point);

    void setPointId(int id);

    Q_DECLARE_PRIVATE(QQuickSinglePointHandler)
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERSINGLEHANDLER_H
