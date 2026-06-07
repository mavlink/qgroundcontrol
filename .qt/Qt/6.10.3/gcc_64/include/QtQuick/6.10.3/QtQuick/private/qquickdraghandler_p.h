// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDRAGHANDLER_H
#define QQUICKDRAGHANDLER_H

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

#include "qquickmultipointhandler_p.h"
#include "qquickdragaxis_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class Q_QUICK_EXPORT QQuickDragHandler : public QQuickMultiPointHandler
{
    Q_OBJECT
    Q_PROPERTY(QQuickDragAxis * xAxis READ xAxis CONSTANT)
    Q_PROPERTY(QQuickDragAxis * yAxis READ yAxis CONSTANT)
#if QT_DEPRECATED_SINCE(6, 2)
    Q_PROPERTY(QVector2D translation READ translation NOTIFY translationChanged)
#endif
    Q_PROPERTY(QVector2D activeTranslation READ activeTranslation NOTIFY translationChanged REVISION(6, 2))
    Q_PROPERTY(QVector2D persistentTranslation READ persistentTranslation WRITE setPersistentTranslation NOTIFY translationChanged REVISION(6, 2))
    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged REVISION(2, 14))
    QML_NAMED_ELEMENT(DragHandler)
    QML_ADDED_IN_VERSION(2, 12)

public:
    enum SnapMode {
        NoSnap = 0,
        SnapAuto,
        SnapIfPressedOutsideTarget,
        SnapAlways
    };
    Q_ENUM(SnapMode)

    explicit QQuickDragHandler(QQuickItem *parent = nullptr);

    void handlePointerEventImpl(QPointerEvent *event) override;

    QQuickDragAxis *xAxis() { return &m_xAxis; }
    QQuickDragAxis *yAxis() { return &m_yAxis; }

#if QT_DEPRECATED_SINCE(6, 2)
    QVector2D translation() const { return activeTranslation(); }
#endif
    QVector2D activeTranslation() const { return QVector2D(QPointF(m_xAxis.activeValue(), m_yAxis.activeValue())); }
    void setActiveTranslation(const QVector2D &trans);
    QVector2D persistentTranslation() const { return QVector2D(QPointF(m_xAxis.persistentValue(), m_yAxis.persistentValue())); }
    void setPersistentTranslation(const QVector2D &trans);
    QQuickDragHandler::SnapMode snapMode() const;
    void setSnapMode(QQuickDragHandler::SnapMode mode);

Q_SIGNALS:
    void translationChanged(QVector2D delta);
    Q_REVISION(2, 14) void snapModeChanged();

protected:
    bool wantsPointerEvent(QPointerEvent *event) override;
    void onActiveChanged() override;
    void onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point) override;

private:
    void ungrab();
    void enforceAxisConstraints(QPointF *localPos);
    QPointF targetCentroidPosition();

private:
    QPointF m_pressTargetPos;   // We must also store the local targetPos, because we cannot deduce
                                // the press target pos from the scene pos in case there was e.g a
                                // flick in one of the ancestors during the drag.

    QQuickDragAxis m_xAxis = {this, u"x"_s};
    QQuickDragAxis m_yAxis = {this, u"y"_s};
    QQuickDragHandler::SnapMode m_snapMode = SnapAuto;
    bool m_pressedInsideParent = false;
    bool m_pressedInsideTarget = false;

    friend class QQuickDragAxis;
};

QT_END_NAMESPACE

#endif // QQUICKDRAGHANDLER_H
