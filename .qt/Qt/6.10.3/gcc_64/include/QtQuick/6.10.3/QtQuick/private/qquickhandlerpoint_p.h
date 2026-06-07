// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKHANDLERPOINT_H
#define QQUICKHANDLERPOINT_H

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

#include "qquickpointerdevicehandler_p.h"

QT_BEGIN_NAMESPACE

class QQuickMultiPointHandler;
class QQuickSinglePointHandler;

class Q_QUICK_EXPORT QQuickHandlerPoint {
    Q_GADGET
    Q_PROPERTY(int id READ id FINAL)
    Q_PROPERTY(QPointingDeviceUniqueId uniqueId READ uniqueId FINAL)
    Q_PROPERTY(QPointF position READ position FINAL)
    Q_PROPERTY(QPointF scenePosition READ scenePosition FINAL)
    Q_PROPERTY(QPointF pressPosition READ pressPosition FINAL)
    Q_PROPERTY(QPointF scenePressPosition READ scenePressPosition FINAL)
    Q_PROPERTY(QPointF sceneGrabPosition READ sceneGrabPosition FINAL)
    Q_PROPERTY(Qt::MouseButtons pressedButtons READ pressedButtons FINAL)
    Q_PROPERTY(Qt::KeyboardModifiers modifiers READ modifiers FINAL)
    Q_PROPERTY(QVector2D velocity READ velocity FINAL)
    Q_PROPERTY(qreal rotation READ rotation FINAL)
    Q_PROPERTY(qreal pressure READ pressure FINAL)
    Q_PROPERTY(QSizeF ellipseDiameters READ ellipseDiameters FINAL)
    Q_PROPERTY(QPointingDevice *device READ device FINAL)
    QML_ANONYMOUS

public:
    QQuickHandlerPoint();

    int id() const { return m_id; }
    Qt::MouseButtons pressedButtons() const { return m_pressedButtons; }
    Qt::KeyboardModifiers modifiers() const { return m_pressedModifiers; }
    QPointF pressPosition() const { return m_pressPosition; }
    QPointF scenePressPosition() const { return m_scenePressPosition; }
    QPointF sceneGrabPosition() const { return m_sceneGrabPosition; }
    QPointF position() const { return m_position; }
    QPointF scenePosition() const { return m_scenePosition; }
    QVector2D velocity() const { return m_velocity; }
    qreal rotation() const { return m_rotation; }
    qreal pressure() const { return m_pressure; }
    QSizeF ellipseDiameters() const { return m_ellipseDiameters; }
    QPointingDeviceUniqueId uniqueId() const { return m_uniqueId; }
    // non-const only because of QML engine limitations (similar to QTBUG-61749)
    QPointingDevice *device() const { return const_cast<QPointingDevice *>(m_device); }
    void localize(QQuickItem *item);

    void reset();
    void reset(const QPointerEvent *event, const QEventPoint &point);
    void reset(const QVector<QQuickHandlerPoint> &points);

private:
    int m_id = -1;
    const QPointingDevice *m_device = QPointingDevice::primaryPointingDevice();
    QPointingDeviceUniqueId m_uniqueId;
    Qt::MouseButtons m_pressedButtons = Qt::NoButton;
    Qt::KeyboardModifiers m_pressedModifiers = Qt::NoModifier;
    QPointF m_position;
    QPointF m_scenePosition;
    QPointF m_pressPosition;
    QPointF m_scenePressPosition;
    QPointF m_sceneGrabPosition;
    QVector2D m_velocity;
    qreal m_rotation = 0;
    qreal m_pressure = 0;
    QSizeF m_ellipseDiameters;
    friend class QQuickMultiPointHandler;
    friend class QQuickSinglePointHandler;
};

QT_END_NAMESPACE

#endif // QQUICKHANDLERPOINT_H
