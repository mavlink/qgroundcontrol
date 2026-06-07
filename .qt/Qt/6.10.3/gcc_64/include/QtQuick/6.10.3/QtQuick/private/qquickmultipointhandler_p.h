// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOINTERMULTIHANDLER_H
#define QQUICKPOINTERMULTIHANDLER_H

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

#include "qquickhandlerpoint_p.h"
#include "qquickpointerdevicehandler_p.h"

QT_BEGIN_NAMESPACE

class QQuickMultiPointHandlerPrivate;

class Q_QUICK_EXPORT QQuickMultiPointHandler : public QQuickPointerDeviceHandler
{
    Q_OBJECT
    Q_PROPERTY(int minimumPointCount READ minimumPointCount WRITE setMinimumPointCount NOTIFY minimumPointCountChanged)
    Q_PROPERTY(int maximumPointCount READ maximumPointCount WRITE setMaximumPointCount NOTIFY maximumPointCountChanged)
    Q_PROPERTY(QQuickHandlerPoint centroid READ centroid NOTIFY centroidChanged)

public:
    explicit QQuickMultiPointHandler(QQuickItem *parent = nullptr, int minimumPointCount = 2, int maximumPointCount = -1);

    int minimumPointCount() const;
    void setMinimumPointCount(int c);

    int maximumPointCount() const;
    void setMaximumPointCount(int maximumPointCount);

    const QQuickHandlerPoint &centroid() const;

Q_SIGNALS:
    void minimumPointCountChanged();
    void maximumPointCountChanged();
    void centroidChanged();

protected:
    struct PointData {
        PointData() : id(0), angle(0) {}
        PointData(quint64 id, qreal angle) : id(id), angle(angle) {}
        quint64 id;
        qreal angle;
    };

    bool wantsPointerEvent(QPointerEvent *event) override;
    void handlePointerEventImpl(QPointerEvent *event) override;
    void onActiveChanged() override;
    void onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point) override;
    QList<QQuickHandlerPoint> &currentPoints();
    QQuickHandlerPoint &mutableCentroid();
    bool hasCurrentPoints(QPointerEvent *event);
    QVector<QEventPoint> eligiblePoints(QPointerEvent *event);
    qreal averageTouchPointDistance(const QPointF &ref);
    qreal averageStartingDistance(const QPointF &ref);
    qreal averageTouchPointAngle(const QPointF &ref);
    qreal averageStartingAngle(const QPointF &ref);
    QVector<PointData> angles(const QPointF &ref) const;
    static qreal averageAngleDelta(const QVector<PointData> &old, const QVector<PointData> &newAngles);
    void acceptPoints(const QVector<QEventPoint> &points);
    bool grabPoints(QPointerEvent *event, const QVector<QEventPoint> &points);
    void moveTarget(QPointF pos);

    Q_DECLARE_PRIVATE(QQuickMultiPointHandler)
};

QT_END_NAMESPACE

#endif // QQUICKPOINTERMULTIHANDLER_H
