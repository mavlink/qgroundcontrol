// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSCROLLBAR_P_P_H
#define QQUICKSCROLLBAR_P_P_H

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

#include <QtQuickTemplates2/private/qquickscrollbar_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

class QQuickFlickable;
class QQuickIndicatorButton;

class QQuickScrollBarPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickScrollBar)

public:
    static QQuickScrollBarPrivate *get(QQuickScrollBar *bar)
    {
        return bar->d_func();
    }

    struct VisualArea
    {
        VisualArea(qreal pos, qreal sz)
            : position(pos), size(sz) { }
        qreal position = 0;
        qreal size = 0;
    };
    VisualArea visualArea() const;

    qreal logicalPosition(qreal position) const;

    void setPosition(qreal position, bool notifyVisualChange = true);
    qreal snapPosition(qreal position) const;
    qreal positionAt(const QPointF &point) const;
    void setInteractive(bool interactive);
    void updateActive();
    void resizeContent() override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    void visualAreaChange(const VisualArea &newVisualArea, const VisualArea &oldVisualArea);

    void updateHover(const QPointF &pos,  std::optional<bool> newHoverState = {});

    QQuickIndicatorButton *decreaseVisual = nullptr;
    QQuickIndicatorButton *increaseVisual = nullptr;
    qreal size = 0;
    qreal position = 0;
    qreal stepSize = 0;
    qreal offset = 0;
    qreal minimumSize = 0;
    bool active = false;
    bool pressed = false;
    bool moving = false;
    bool interactive = true;
    bool explicitInteractive = false;
    Qt::Orientation orientation = Qt::Vertical;
    QQuickScrollBar::SnapMode snapMode = QQuickScrollBar::NoSnap;
    QQuickScrollBar::Policy policy = QQuickScrollBar::AsNeeded;
};

class QQuickScrollBarAttachedPrivate : public QObjectPrivate,
                                       public QSafeQuickItemChangeListener<QQuickScrollBarAttachedPrivate>
{
public:
    static QQuickScrollBarAttachedPrivate *get(QQuickScrollBarAttached *attached)
    {
        return attached->d_func();
    }

    void setFlickable(QQuickFlickable *flickable);

    void initHorizontal();
    void initVertical();
    void cleanupHorizontal();
    void cleanupVertical();
    void activateHorizontal();
    void activateVertical();
    void scrollHorizontal();
    void scrollVertical();
    void mirrorVertical();

    void layoutHorizontal(bool move = true);
    void layoutVertical(bool move = true);

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    QQuickFlickable *flickable = nullptr;
    QQuickScrollBar *horizontal = nullptr;
    QQuickScrollBar *vertical = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKSCROLLBAR_P_P_H
