// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPINCHHANDLER_H
#define QQUICKPINCHHANDLER_H

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

#include "qquickmultipointhandler_p.h"
#include <private/qquicktranslate_p.h>
#include "qquickdragaxis_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class Q_QUICK_EXPORT QQuickPinchHandler : public QQuickMultiPointHandler
{
    Q_OBJECT

    Q_PROPERTY(QQuickDragAxis *scaleAxis READ scaleAxis CONSTANT)
#if QT_DEPRECATED_SINCE(6, 5)
    Q_PROPERTY(qreal minimumScale READ minimumScale WRITE setMinimumScale NOTIFY minimumScaleChanged)
    Q_PROPERTY(qreal maximumScale READ maximumScale WRITE setMaximumScale NOTIFY maximumScaleChanged)
    Q_PROPERTY(qreal scale READ scale NOTIFY updated)
#endif
    Q_PROPERTY(qreal activeScale READ activeScale NOTIFY scaleChanged)
    Q_PROPERTY(qreal persistentScale READ persistentScale WRITE setPersistentScale NOTIFY scaleChanged)

    Q_PROPERTY(QQuickDragAxis *rotationAxis READ rotationAxis CONSTANT)
#if QT_DEPRECATED_SINCE(6, 5)
    Q_PROPERTY(qreal minimumRotation READ minimumRotation WRITE setMinimumRotation NOTIFY minimumRotationChanged)
    Q_PROPERTY(qreal maximumRotation READ maximumRotation WRITE setMaximumRotation NOTIFY maximumRotationChanged)
    Q_PROPERTY(qreal rotation READ rotation NOTIFY updated)
#endif
    Q_PROPERTY(qreal activeRotation READ activeRotation NOTIFY rotationChanged)
    Q_PROPERTY(qreal persistentRotation READ persistentRotation WRITE setPersistentRotation NOTIFY rotationChanged)

    Q_PROPERTY(QQuickDragAxis * xAxis READ xAxis CONSTANT)
    Q_PROPERTY(QQuickDragAxis * yAxis READ yAxis CONSTANT)
#if QT_DEPRECATED_SINCE(6, 5)
    Q_PROPERTY(QVector2D translation READ translation NOTIFY updated)
#endif
    Q_PROPERTY(QPointF activeTranslation READ activeTranslation NOTIFY translationChanged REVISION(6, 5))
    Q_PROPERTY(QPointF persistentTranslation READ persistentTranslation WRITE setPersistentTranslation NOTIFY translationChanged REVISION(6, 5))

    QML_NAMED_ELEMENT(PinchHandler)
    QML_ADDED_IN_VERSION(2, 12)

public:
    explicit QQuickPinchHandler(QQuickItem *parent = nullptr);

    QQuickDragAxis *xAxis() { return &m_xAxis; }
    QQuickDragAxis *yAxis() { return &m_yAxis; }
#if QT_DEPRECATED_SINCE(6, 5)
    QVector2D translation() const { return QVector2D(activeTranslation()); }
#endif
    QPointF activeTranslation() const { return QPointF(m_xAxis.activeValue(), m_yAxis.activeValue()); }
    QPointF persistentTranslation() const { return QPointF(m_xAxis.persistentValue(), m_yAxis.persistentValue()); }
    void setPersistentTranslation(const QPointF &trans);

    QQuickDragAxis *scaleAxis() { return &m_scaleAxis; }
#if QT_DEPRECATED_SINCE(6, 5)
    qreal minimumScale() const { return m_scaleAxis.minimum(); }
    void setMinimumScale(qreal minimumScale);
    qreal maximumScale() const { return m_scaleAxis.maximum(); }
    void setMaximumScale(qreal maximumScale);
    qreal scale() const { return persistentScale(); }
#endif
    qreal activeScale() const { return m_scaleAxis.activeValue(); }
    void setActiveScale(qreal scale);
    qreal persistentScale() const { return m_scaleAxis.persistentValue(); }
    void setPersistentScale(qreal scale);

    QQuickDragAxis *rotationAxis() { return &m_rotationAxis; }
#if QT_DEPRECATED_SINCE(6, 5)
    qreal minimumRotation() const { return m_rotationAxis.minimum(); }
    void setMinimumRotation(qreal minimumRotation);
    qreal maximumRotation() const { return m_rotationAxis.maximum(); }
    void setMaximumRotation(qreal maximumRotation);
    qreal rotation() const { return activeRotation(); }
#endif
    qreal activeRotation() const { return m_rotationAxis.activeValue(); }
    void setActiveRotation(qreal rot);
    qreal persistentRotation() const { return m_rotationAxis.persistentValue(); }
    void setPersistentRotation(qreal rot);

Q_SIGNALS:
    void minimumScaleChanged();
    void maximumScaleChanged();
    void minimumRotationChanged();
    void maximumRotationChanged();
    void updated();
    void scaleChanged(qreal delta);
    void rotationChanged(qreal delta);
    void translationChanged(QVector2D delta);

protected:
    bool wantsPointerEvent(QPointerEvent *event) override;
    void onActiveChanged() override;
    void handlePointerEventImpl(QPointerEvent *event) override;

private:
    QQuickDragAxis m_xAxis = {this, u"x"_s};
    QQuickDragAxis m_yAxis = {this, u"y"_s};
    QQuickDragAxis m_scaleAxis = {this, u"scale"_s, 1};
    QQuickDragAxis m_rotationAxis = {this, u"rotation"_s};

    // internal
    qreal m_startDistance = 0;
    qreal m_accumulatedStartCentroidDistance = 0;
    QPointF m_startTargetPos;
    QVector<PointData> m_startAngles;
    QQuickMatrix4x4 m_transform;
};

QT_END_NAMESPACE

#endif // QQUICKPINCHHANDLER_H
